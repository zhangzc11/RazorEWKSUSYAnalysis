#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <assert.h>
//ROOT INCLUDES
#include <TFile.h>
#include <TROOT.h>
#include <TF1.h>
#include <RooArgSet.h>
#include <RooAbsArg.h>
#include <RooAbsCollection.h>
#include <RooAbsReal.h>
#include <RooAbsPdf.h>
#include <RooRealVar.h>
#include <RooFitResult.h>
#include <RooWorkspace.h>
//LOCAL INCLUDES
#include "HggRazorSystematics.hh"
#include "CommandLineInput.hh"
#include "DefinePdfs.hh"

const bool _debug = false;

float GetNs( std::string fname, int bin, std::string cat = "highpt" );
float GetNsErr( std::string fname, int bin, std::string cat = "highpt" );
float GetNbkg( std::string fname, std::string f1, int bin, bool _err = false, std::string cat = "highpt" );

//----------------------------------------------                                                                                                                             
//Load Binning                                                                                                                                                               
//----------------------------------------------                                                                                                                             
std::vector<float*> SetBinning(std::vector<Bin> bins, std::string category)
{

  std::vector<float*> myVec;
  for(int i=0; i<bins.size(); ++i) {
    float *tmpbin = new float[5];
    if (bins[i].box == category) {
      tmpbin[0] = bins[i].x1;
      tmpbin[1] = bins[i].y1;
      tmpbin[2] = bins[i].x2;
      tmpbin[3] = bins[i].y2;
      tmpbin[4] = bins[i].bin;
      myVec.push_back(tmpbin);
    }
  }

  return myVec;
};



//----------------
//Static Variables
//----------------
float HggRazorSystematics::NR_kf = 1.0;
int   HggRazorSystematics::n_PdfSys = 60;


int main( int argc, char* argv[] )
{

  gROOT->Reset();

  std::map<Bin, std::string> myMap;
  std::map<std::string, Bin> myMap2;
  
  std::vector<Bin> binVector;
  std::ifstream input( "data/HggRazor2016Binning.txt", std::fstream::in );
  if ( input.is_open() )
    {
      float x1, x2, y1, y2;
      int binN;
      std::string box, f1;
      while ( input.good() )
	{
	  input >> binN >> x1 >> x2 >> y1 >> y2 >> box >> f1;
	  if ( input.eof() ) break;
	  Bin mybin;
	  mybin.bin = binN;
	  mybin.f1 = f1;
	  mybin.box = box;
	  mybin.x1 = x1;
	  mybin.x2 = x2;
	  mybin.y1 = y1;
	  mybin.y2 = y2;
	  std::stringstream ss;
	  ss << box << "_" << x1 << "-" << x2 << "_" << y1 << "-" << y2;
	  binVector.push_back(mybin);
	  if ( myMap.find( mybin ) == myMap.end() ) myMap[mybin] = f1;
	  if ( myMap2.find( ss.str() ) == myMap2.end() ) myMap2[ss.str()] = mybin;
	}
    }
  else
    {
      std::cout << "Unable to open binning lookup table" << std::endl;
    }

  for ( auto tmp : myMap2 )
    {
      std::cout << tmp.first << " " << tmp.second.f1 << std::endl;
    }
  
  std::string inputList = ParseCommandLine( argc, argv, "-inputList=" );
  if (  inputList == "" )
    {
      std::cerr << "[ERROR]: please provide an input list file using --inputList=<path_to_list_file>" << std::endl;
      return -1;
    }
  //-----------------
  //Selection TString
  //-----------------
  std::string categoryMode = ParseCommandLine( argc, argv, "-category=" );
  if (  categoryMode == "" )
    {
      std::cerr << "[ERROR]: please provide the category. Use --category=<highpt,hzbb,highres,lowres>" << std::endl;
      return -1;
    }
  
  //-----------------
  //Lumi Normalization
  //-----------------
  double lumi = 0;
  std::string lumiString = ParseCommandLine( argc, argv, "-lumi=" );
  if (  lumiString == "" )
    {
      std::cerr << "[ERROR]: please provide the luminosity. For example, use --lumi=2000" << std::endl;
      return -1;
    }
  lumi = float(atoi(lumiString.c_str()));
  std::cout << "[INFO] : Using Luminosity = " << lumi << "\n";

  //------------------------
  //fit result directory
  //------------------------
  std::string fitResultDir = ParseCommandLine( argc, argv, "-fitResultDir=" );
  if (  fitResultDir == "" )
    {
      std::cerr << "[ERROR]: please provide a fit result directory. For example, use --fitResultDir=/afs/cern.ch/work/c/cpena/public/CMSSW_7_6_3/src/RazorEWKSUSYAnalysis/HggRazor/PlottingAndSystematic" << std::endl;
      return -1;
    }
  std::cout << "[INFO] : Using fitResultDir = " << fitResultDir << "\n";

  //-----------------
  //Analysis Tag
  //-----------------
  std::string analysisTag = ParseCommandLine( argc, argv, "-analysisTag=" );
  if ( analysisTag == "" )
    {
      std::cerr << "[ERROR]: please provide the analysisTag. Use --analysisTag=<Razor2015_76X,Razor2016_80X>" << std::endl;
      return -1;
    } 
  

  TString cut = "mGammaGamma > 103. && mGammaGamma < 160. && pho1passIso == 1 && pho2passIso == 1 && pho1passEleVeto == 1 && pho2passEleVeto == 1 && abs(pho1Eta) <1.48 && abs(pho2Eta)<1.48 && (pho1Pt>40||pho2Pt>40)  && pho1Pt> 25. && pho2Pt>25.";
  TString categoryCutString;

  if (categoryMode == "highpt") categoryCutString = " && pTGammaGamma >= 110 ";
  else if (categoryMode == "hzbb") categoryCutString = " && pTGammaGamma < 110 && ( abs(mbbH_L-125.) < 15. || ( abs(mbbH_L-125.) >= 15. && abs(mbbZ_L-91.) < 15 ) )";
  else if (categoryMode == "highres") categoryCutString = " && pTGammaGamma < 110 && abs(mbbH_L-125.) >= 15 && abs(mbbZ_L-91.) >= 15 && sigmaMoverM < 0.0085";
  else if (categoryMode == "lowres") categoryCutString  = " && pTGammaGamma < 110 && abs(mbbH_L-125.) >= 15 && abs(mbbZ_L-91.) >= 15 && sigmaMoverM >= 0.0085 ";
  else if (categoryMode == "inclusive") categoryCutString = "";

  //TString triggerCut = " && ( HLTDecision[82] || HLTDecision[83] || HLTDecision[93] ) ";
  //TString metFilterCut = " && (Flag_HBHENoiseFilter == 1 && Flag_CSCTightHaloFilter == 1 && Flag_goodVertices == 1 && Flag_eeBadScFilter == 1 && Flag_HBHEIsoNoiseFilter == 1)";
  
  
  TString triggerCut = "";
  TString metFilterCut = "";
 if ( analysisTag == "Razor2015_76X" ) {
   cut = cut + categoryCutString + triggerCut+ metFilterCut;  
  } else if ( analysisTag == "Razor2016_80X" ) {
    //for 80X MC, trigger table doesn't exist. so don't apply triggers.
   cut = cut + categoryCutString + metFilterCut;
  } else {
    std::cout << "Analysis Tag " << analysisTag << " not recognized. Error!\n";
    return -1;
  }
  std::cout << "[INFO] : Using Analysis Tag: " << analysisTag  << "\n";

  std::cout << "===========================================================================" << std::endl;
  std::cout << "[INFO]: cut--> " << cut << std::endl;
  std::cout << "===========================================================================" << std::endl;



  
  std::ifstream ifs( inputList, std::ifstream::in );
  assert(ifs);

  std::vector<std::pair<float,float>> facScaleSys;
  std::vector<std::pair<float,float>> renScaleSys;
  std::vector<std::pair<float,float>> facRenScaleSys;

  std::vector<float*> myVectBinning;
  myVectBinning = SetBinning(binVector, categoryMode );
  
  TH2Poly* nominal[4];
  nominal[0] = new TH2Poly("nominal_SMH_0", "", 150, 10000, 0, 1 );
  nominal[1] = new TH2Poly("nominal_SMH_1", "", 150, 10000, 0, 1 );
  nominal[2] = new TH2Poly("nominal_SMH_2", "", 150, 10000, 0, 1 );
  nominal[3] = new TH2Poly("nominal_SMH_3", "", 150, 10000, 0, 1 );

  TH2Poly* nominalErr[4];
  nominalErr[0] = new TH2Poly("nominalErr_SMH_0", "", 150, 10000, 0, 1 );
  nominalErr[1] = new TH2Poly("nominalErr_SMH_1", "", 150, 10000, 0, 1 );
  nominalErr[2] = new TH2Poly("nominalErr_SMH_2", "", 150, 10000, 0, 1 );
  nominalErr[3] = new TH2Poly("nominalErr_SMH_3", "", 150, 10000, 0, 1 );
  
  TH2Poly* totalErr[4];
  totalErr[0] = new TH2Poly("totalErr_SMH_0", "", 150, 10000, 0, 1 );
  totalErr[1] = new TH2Poly("totalErr_SMH_1", "", 150, 10000, 0, 1 );
  totalErr[2] = new TH2Poly("totalErr_SMH_2", "", 150, 10000, 0, 1 );
  totalErr[3] = new TH2Poly("totalErr_SMH_3", "", 150, 10000, 0, 1 );

  
  TH2Poly* nominalS = new TH2Poly("nominal_Signal", "", 150, 10000, 0, 1 );

  TH2Poly* facScaleUp    = new TH2Poly("facScaleUp", "", 150, 10000, 0, 1 );
  TH2Poly* facScaleDown  = new TH2Poly("facScaleDown", "", 150, 10000, 0, 1 );
  TH2Poly* facScaleUpS   = new TH2Poly("facScaleUpS", "", 150, 10000, 0, 1 );
  TH2Poly* facScaleDownS = new TH2Poly("facScaleDownS", "", 150, 10000, 0, 1 );
  
  TH2Poly* renScaleUp    = new TH2Poly("renScaleUp", "", 150, 10000, 0, 1 );
  TH2Poly* renScaleDown  = new TH2Poly("renScaleDown", "", 150, 10000, 0, 1 );
  TH2Poly* renScaleUpS   = new TH2Poly("renScaleUpS", "", 150, 10000, 0, 1 );
  TH2Poly* renScaleDownS = new TH2Poly("renScaleDownS", "", 150, 10000, 0, 1 );
  
  TH2Poly* facRenScaleUp    = new TH2Poly("facRenScaleUp", "", 150, 10000, 0, 1 );
  TH2Poly* facRenScaleDown  = new TH2Poly("facRenScaleDown", "", 150, 10000, 0, 1 );
  TH2Poly* facRenScaleUpS   = new TH2Poly("facRenScaleUpS", "", 150, 10000, 0, 1 );
  TH2Poly* facRenScaleDownS = new TH2Poly("facRenScaleDownS", "", 150, 10000, 0, 1 );
  
  TH2Poly* JesUp    = new TH2Poly("JesUp", "", 150, 10000, 0, 1 );
  TH2Poly* JesDown  = new TH2Poly("JesDown", "", 150, 10000, 0, 1 );
  TH2Poly* JesUpS   = new TH2Poly("JesUpS", "", 150, 10000, 0, 1 );//signal
  TH2Poly* JesDownS = new TH2Poly("JesDownS", "", 150, 10000, 0, 1 );//signal

  TH2Poly* btagUp    = new TH2Poly("btagUp", "", 150, 10000, 0, 1 );
  TH2Poly* btagDown  = new TH2Poly("btagDown", "", 150, 10000, 0, 1 );
  TH2Poly* btagUpS   = new TH2Poly("btagUpS", "", 150, 10000, 0, 1 );//signal
  TH2Poly* btagDownS = new TH2Poly("btagDownS", "", 150, 10000, 0, 1 );//signal

  TH2Poly* misstagUp    = new TH2Poly("misstagUp", "", 150, 10000, 0, 1 );
  TH2Poly* misstagDown  = new TH2Poly("misstagDown", "", 150, 10000, 0, 1 );
  TH2Poly* misstagUpS   = new TH2Poly("misstagUpS", "", 150, 10000, 0, 1 );//signal
  TH2Poly* misstagDownS = new TH2Poly("misstagDownS", "", 150, 10000, 0, 1 );//signal
  
  
  TH2Poly* pdf[60];
  TH2Poly* pdfS[60];
  for ( int i = 0; i < 60; i++ )
    {
      TString pdfName = Form("pdf%d", i);
      pdf[i]  = new TH2Poly( pdfName, "", 150, 10000, 0, 1 );
      pdfS[i] = new TH2Poly( pdfName+"S", "", 150, 10000, 0, 1 );
    }
  std::map< std::pair<float,float>, float > sysMap;
  for ( auto tmp : myVectBinning )
    {
      nominal[0]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      nominal[1]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      nominal[2]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      nominal[3]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );

      nominalErr[0]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      nominalErr[1]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      nominalErr[2]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      nominalErr[3]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );

      totalErr[0]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      totalErr[1]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      totalErr[2]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      totalErr[3]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      
      
      nominalS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facScaleUp->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facScaleUpS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facScaleDown->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facScaleDownS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      renScaleUp->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      renScaleUpS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      renScaleDown->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      renScaleDownS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facRenScaleUp->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facRenScaleUpS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facRenScaleDown->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      facRenScaleDownS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      JesUp->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      JesUpS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      JesDown->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      JesDownS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );

      //btag
      btagUp->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      btagUpS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      btagDown->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      btagDownS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      //misstag
      misstagUp->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      misstagUpS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      misstagDown->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      misstagDownS->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
      //pdf
      for( int i = 0; i < 60; i++ )
	{
	  pdf[i]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
	  pdfS[i]->AddBin( tmp[0], tmp[1], tmp[2], tmp[3] );
	}
    }

  std::map< std::string, TH2Poly* > smhMapNominal;
  std::map< std::string, TH2Poly* > smhMapNominalErr;
  std::map< std::string, TH2Poly* > smhMapTotalErr;
  std::string process, rootFileName;
  int ctr = 0;
  while ( ifs.good() )
    {
      ifs >> process >> rootFileName;
      if ( ifs.eof() ) break;
      if ( process.find("#") != std::string::npos ) continue;
      if ( _debug ) std::cout << process << " " << rootFileName << std::endl;
      TFile* fin = new TFile( rootFileName.c_str(), "READ");
      //std::cout << "[INFO]: " << process <<" file: " << rootFileName << " " << ctr << std::endl;
      assert( fin );
      if ( _debug ) std::cout << "[INFO]: file: " << rootFileName << " passed check\n\n"<< std::endl;
      
      //------------------------
      //Getting TTree and Histos
      //------------------------
      TTree* tree = (TTree*)fin->Get("HggRazor");
      assert( tree );
      TH1F* NEvents = (TH1F*)fin->Get("NEvents");
      if ( process != "signal" ) assert( NEvents );
      TH1F* SumScaleWeights   = (TH1F*)fin->Get("SumScaleWeights");
      if ( process != "signal" ) assert( SumScaleWeights );
      TH1F* SumPdfWeights   = (TH1F*)fin->Get("SumPdfWeights");
      if ( process != "signal" ) assert( SumPdfWeights );
      
      TFile* tmp = new TFile("tmp.root", "RECREATE");
      TTree* cutTree = tree->CopyTree( cut );
      TString currentProcess = process.c_str();

      //---------------------------
      //Create HggSystematic object
      //---------------------------
      HggRazorSystematics* hggSys = new HggRazorSystematics( cutTree, currentProcess, categoryMode, analysisTag, false, false );
      hggSys->SetLumi(lumi);
      //hggSys->PrintBinning();
      //hggSys->SetBinningMap( binningMap );
      //hggSys->PrintBinning();
      hggSys->SetBinningVector( myVectBinning );
      hggSys->InitMrRsqTH2Poly( 1 );
      hggSys->SetNeventsHisto( NEvents );
      hggSys->SetFacScaleWeightsHisto( SumScaleWeights );
      hggSys->SetPdfWeightsHisto( SumPdfWeights );
      hggSys->Loop();
      for ( auto tmp: myVectBinning )
	{
	  int bin = nominal[ctr]->FindBin( tmp[0]+10, tmp[1]+0.0001 );
	  if ( currentProcess == "signal" )
	    {
	      nominalS->SetBinContent( bin, hggSys->GetNominalYield( tmp[0], tmp[1] ) );
	      //facScale
	      std::pair<float, float> facSys = hggSys->GetFacScaleSystematic( tmp[0], tmp[1] );
	      facScaleUpS->SetBinContent( bin, facSys.first );
	      facScaleDownS->SetBinContent( bin, facSys.second );
	      //renScale
	      facSys = hggSys->GetRenScaleSystematic( tmp[0], tmp[1] );
	      renScaleUpS->SetBinContent( bin, facSys.first );
	      renScaleDownS->SetBinContent( bin, facSys.second );
	      //facRenScale
	      facSys = hggSys->GetFacRenScaleSystematic( tmp[0], tmp[1] );
	      facRenScaleUpS->SetBinContent( bin,  facSys.first );
	      facRenScaleDownS->SetBinContent( bin, facSys.second );
	      //JES
	      facSys = hggSys->GetJesSystematic( tmp[0], tmp[1] );
	      JesUpS->SetBinContent( bin, facSys.first );
	      JesDownS->SetBinContent( bin, facSys.second );
	      //btag
	      facSys = hggSys->GetBtagSystematic( tmp[0], tmp[1] );
	      btagUpS->SetBinContent( bin, facSys.first );
	      btagDownS->SetBinContent( bin, facSys.second );
	      //misstag
	      facSys = hggSys->GetMisstagSystematic( tmp[0], tmp[1] );
	      misstagUpS->SetBinContent( bin, facSys.first );
	      misstagDownS->SetBinContent( bin, facSys.second );
	      //PDF
	      for ( int ipdf = 0; ipdf < 60; ipdf++ )
		{
		  pdfS[ipdf]->SetBinContent( bin, hggSys->GetPdfSystematic( ipdf, tmp[0], tmp[1] ) );
		  //std::cout << "mr: " << tmp[0] << " rsq: " << tmp[1] << "; pdf: " << hggSys->GetPdfSystematic( ipdf, tmp[0], tmp[1] ) << std::endl;
		}
	    }
	  else
	    {
	      float nom = hggSys->GetNominalYield( tmp[0], tmp[1] );
	      nominal[ctr]->SetBinContent( bin, hggSys->GetNominalYield( tmp[0], tmp[1] ) );
	      nominalErr[ctr]->SetBinContent( bin, hggSys->GetNominalError( tmp[0], tmp[1] ) );
	      //facScale
	      std::pair<float, float> facSys = hggSys->GetFacScaleSystematic( tmp[0], tmp[1] );
	      float maxSys = std::max( fabs(facSys.first) , fabs(facSys.second) );
	      totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      facScaleUp->SetBinContent( bin, sqrt( pow( facScaleUp->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      facScaleDown->SetBinContent( bin, sqrt( pow( facScaleDown->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      //renScale
	      facSys = hggSys->GetRenScaleSystematic( tmp[0], tmp[1] );
	      maxSys = std::max( fabs(facSys.first) , fabs(facSys.second) );
	      totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );

	      renScaleUp->SetBinContent( bin, sqrt( pow( renScaleUp->GetBinContent(bin), 2 ) + pow( maxSys, 2) ) );
	      renScaleDown->SetBinContent( bin, sqrt( pow( renScaleDown->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      //facRenScale
	      facSys = hggSys->GetFacRenScaleSystematic( tmp[0], tmp[1] );
	      maxSys = std::max( fabs(facSys.first) , fabs(facSys.second) );
	      totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      facRenScaleUp->SetBinContent( bin, sqrt( pow( facRenScaleUp->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      facRenScaleDown->SetBinContent( bin, sqrt( pow( facRenScaleDown->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      //JES
	      facSys = hggSys->GetJesSystematic( tmp[0], tmp[1] );
	      maxSys = std::max( fabs(facSys.first) , fabs(facSys.second) );
	      totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      JesUp->SetBinContent( bin, JesUp->GetBinContent(bin) + facSys.first );
	      JesDown->SetBinContent( bin, JesDown->GetBinContent(bin) + facSys.second );
	      //btag
	      facSys = hggSys->GetBtagSystematic( tmp[0], tmp[1] );
	      maxSys = std::max( fabs(facSys.first) , fabs(facSys.second) );
	      totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      btagUp->SetBinContent( bin, btagUp->GetBinContent(bin) + facSys.first );
	      btagDown->SetBinContent( bin, btagDown->GetBinContent(bin) + facSys.second );
	      //misstag
	      facSys = hggSys->GetMisstagSystematic( tmp[0], tmp[1] );
	      maxSys = std::max( fabs(facSys.first) , fabs(facSys.second) );
	      totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( maxSys, 2 ) ) );
	      misstagUp->SetBinContent( bin, misstagUp->GetBinContent(bin) + facSys.first );
	      misstagDown->SetBinContent( bin, misstagDown->GetBinContent(bin) + facSys.second );
	      //PDF
	      for ( int ipdf = 0; ipdf < 60; ipdf++ )
		{
		  pdf[ipdf]->SetBinContent( bin, sqrt( pow( pdf[ipdf]->GetBinContent(bin), 2 ) + pow( hggSys->GetPdfSystematic( ipdf, tmp[0], tmp[1] ), 2 ) ) );
		  totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( hggSys->GetPdfSystematic( ipdf, tmp[0], tmp[1] ), 2 ) ) );
	      //std::cout << "mr: " << tmp[0] << " rsq: " << tmp[1] << "; pdf: " << hggSys->GetPdfSystematic( ipdf, tmp[0], tmp[1] ) << std::endl;
		}
	      //totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( nom*0.04, 2 ) ) );//lumi
	      //totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( nom*0.05, 2 ) ) );//photon selection
	      //totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( nom*0.067, 2 ) ) );//scale variation
	      //totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( nom*0.057, 2 ) ) );//PDF uncertainty
	      //if ( categoryMode == "hzbb" )  totalErr[ctr]->SetBinContent( bin,  sqrt( pow( totalErr[ctr]->GetBinContent(bin), 2 ) + pow( nom*0.04, 2 ) ) );//btag
	    }     
	}

      
      if ( process != "signal" && process != "" )
	{
	  smhMapNominal[process] = nominal[ctr];
	  smhMapNominalErr[process] = nominalErr[ctr];
	  smhMapTotalErr[process] = totalErr[ctr];
	  if(ctr < 3 )ctr++;
	}
      
      hggSys->WriteOutput( "histoMR_Rsq" );
      delete hggSys;
      if ( _debug ) std::cout << "deleted hggSys" << std::endl;
      //delete tmp;
      if ( _debug ) std::cout << "deleted tmp File" << std::endl;
    }



  std::cout << "\\begin{table*}[htb]\n\\footnotesize\n\\begin{center}\n\\caption{";
  std::cout << categoryMode << " category binning. SM Higgs, and signal expected yields for an integrated luminosity correspondint to 6.3~$\\mathrm{fb}^{-1}$";
  std::cout << "\\label{tab:binning-highpt}}\n\\def\\arraystretch{1.5}\n\\begin{tabular}{|c|c|c|c|c|c|c|}\n\\hline\n$\\mathrm{M_{R}} (GeV)\\otimes\\mathrm{R^{2}}$";
  std::cout << " & ggH & ttH & vbfH & vH & non-resonant & Signal\\\\" << std::endl;
  std::cout << "\\hline" << std::endl;
  for ( auto tmp: myVectBinning )
    {
      int bin   = smhMapNominal["ggH"]->FindBin( tmp[0]+10, tmp[1]+0.0001 );
      float nom_ggH = smhMapNominal["ggH"]->GetBinContent( bin );
      float nom_ttH = smhMapNominal["ttH"]->GetBinContent( bin );
      float nom_vbfH = smhMapNominal["vbfH"]->GetBinContent( bin );
      float nom_vH = smhMapNominal["vH"]->GetBinContent( bin );
      float nom_s  = nominalS->GetBinContent( bin );

      /*
      //Adding systematic uncertainties
      float nom_ggH_U = sqrt( pow(smhMapTotalErr["ggH"]->GetBinContent( bin ),2) + pow(nom_ggH*0.04, 2) + pow(nom_ggH*0.05,2) + pow(nom_ggH*0.079,2) + pow(nom_ggH*0.071,2) );
      if ( categoryMode == "hzbb" ) nom_ggH_U = sqrt( pow(nom_ggH_U,2) + pow(nom_ggH*0.04,2) );
      float nom_ttH_U = sqrt( pow(smhMapTotalErr["ttH"]->GetBinContent( bin ),2) + pow(nom_ttH*0.04, 2) + pow(nom_ttH*0.05,2) + pow(nom_ttH*0.093,2) + pow(nom_ttH*0.088,2) );
      if ( categoryMode == "hzbb" ) nom_ttH_U = sqrt( pow(nom_ttH_U,2) + pow(nom_ttH*0.04,2) );
      float nom_vbfH_U = sqrt( pow(smhMapTotalErr["vbfH"]->GetBinContent( bin ),2) + pow(nom_vbfH*0.04, 2) + pow(nom_vbfH*0.05,2) + pow(nom_vbfH*0.007,2) + pow(nom_vbfH*0.032,2) );
      if ( categoryMode == "hzbb" ) nom_vbfH_U = sqrt( pow(nom_vbfH_U,2) + pow(nom_vbfH*0.04,2) );
      float nom_vH_U = sqrt( pow(smhMapTotalErr["vH"]->GetBinContent( bin ),2) + pow(nom_vH*0.04, 2) + pow(nom_vH*0.05,2) + pow(nom_vH*0.038,2) + pow(nom_vH*0.022,2) );
      if ( categoryMode == "hzbb" ) nom_vH_U = sqrt( pow(nom_vH_U,2) + pow(nom_vH*0.04,2) );
      float nom_s_U  = nominalS->GetBinError( bin );
      */
      float nom_ggH_U = sqrt( smhMapNominalErr["ggH"]->GetBinContent( bin ) );
      float nom_ttH_U = sqrt( smhMapNominalErr["ttH"]->GetBinContent( bin ) );
      float nom_vbfH_U = sqrt( smhMapNominalErr["vbfH"]->GetBinContent( bin ) );
      float nom_vH_U = sqrt( smhMapNominalErr["vH"]->GetBinContent( bin ) );
      float nom_s_U  = nominalS->GetBinError( bin );
      //----------------------------
      //Key string to find bin
      //----------------------------
      std::stringstream ss;
      ss << categoryMode << "_" << tmp[0] << "-" << tmp[2] << "_" << tmp[1] << "-" << tmp[3]; 
      std::stringstream ss_fn;
      if ( categoryMode ==  "lowres" ) myMap2[ss.str()].bin = myMap2[ss.str()].bin - 5;
      ss_fn << fitResultDir << "/mlfit_bin" << myMap2[ss.str()].bin << ".root";
      
      float Ns = GetNs( ss_fn.str(),  myMap2[ss.str()].bin, categoryMode );
      float NsErr = GetNsErr( ss_fn.str(),  myMap2[ss.str()].bin, categoryMode );
      float Nbkg = GetNbkg( ss_fn.str(),  myMap2[ss.str()].f1, myMap2[ss.str()].bin, false, categoryMode );
      float NbkgErr = GetNbkg( ss_fn.str(),  myMap2[ss.str()].f1, myMap2[ss.str()].bin, true, categoryMode );
      
      TString line = Form("%0.f-%0.f $\\otimes$ %.3f-%.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f \\\\",
			  tmp[0], tmp[2], tmp[1], tmp[3], nom_ggH, nom_ggH_U, nom_ttH, nom_ttH_U, nom_vbfH, nom_vbfH_U, nom_vH, nom_vH_U, Nbkg, NbkgErr, Ns, NsErr);
      std::cout << line << std::endl;
    }
  
  std::cout << "\\hline\n\\end{tabular}\n\\end{center}\n\\end{table*}" << std::endl;

     
  std::cout << "\\begin{table*}[htb]\n\\footnotesize\n\\begin{center}\n\\caption{";
  std::cout << categoryMode << " category binning. SM Higgs, and signal expected yields for an integrated luminosity correspondint to 6.3~$\\mathrm{fb}^{-1}$";
  std::cout << "\\label{tab:binning-highpt}}\n\\def\\arraystretch{1.5}\n\\begin{tabular}{|c|c|c|c|}\n\\hline\n$\\mathrm{M_{R}} (GeV)\\otimes\\mathrm{R^{2}}$";
  std::cout << " & higgs & non-resonant & Signal\\\\" << std::endl;
  std::cout << "\\hline" << std::endl;
  for ( auto tmp: myVectBinning )
    {
      int bin   = smhMapNominal["ggH"]->FindBin( tmp[0]+10, tmp[1]+0.0001 );
      float nom_ggH = smhMapNominal["ggH"]->GetBinContent( bin );
      float nom_ttH = smhMapNominal["ttH"]->GetBinContent( bin );
      float nom_vbfH = smhMapNominal["vbfH"]->GetBinContent( bin );
      float nom_vH = smhMapNominal["vH"]->GetBinContent( bin );
      float nom_s  = nominalS->GetBinContent( bin );
      
      float totalsmh =  nom_ggH + nom_ttH + nom_vbfH + nom_vH;

      float nom_ggH_U  = smhMapTotalErr["ggH"]->GetBinContent( bin );
      float nom_ttH_U  = smhMapTotalErr["ttH"]->GetBinContent( bin );
      float nom_vbfH_U = smhMapTotalErr["vbfH"]->GetBinContent( bin );
      float nom_vH_U   = smhMapTotalErr["vH"]->GetBinContent( bin );
      float nom_s_U    = nominalS->GetBinError( bin );
      
      float totalUn = sqrt( pow(nom_ggH_U,2) + pow(nom_ttH_U,2) + pow(nom_vbfH_U,2) + pow(nom_vH_U,2) );
      totalUn = sqrt(pow(totalUn,2) + pow(totalsmh*0.04,2) + pow(totalsmh*0.05,2) + pow(totalsmh*0.067,2) + pow(totalsmh*0.057,2));
      if ( categoryMode == "hzbb" ) totalUn = sqrt( pow(totalUn,2) + pow(totalsmh*0.04,2) );
      
      //----------------------------
      //Key string to find bin
      //----------------------------
      std::stringstream ss;
      ss << categoryMode << "_" << tmp[0] << "-" << tmp[2] << "_" << tmp[1] << "-" << tmp[3]; 
      std::stringstream ss_fn;
      ss_fn << fitResultDir << "/mlfit_bin" << myMap2[ss.str()].bin << ".root";
      
      float Ns = GetNs( ss_fn.str(),  myMap2[ss.str()].bin, categoryMode );
      float NsErr = GetNsErr( ss_fn.str(),  myMap2[ss.str()].bin, categoryMode );
      float Nbkg = GetNbkg( ss_fn.str(),  myMap2[ss.str()].f1, myMap2[ss.str()].bin, false, categoryMode );
      float NbkgErr = GetNbkg( ss_fn.str(),  myMap2[ss.str()].f1, myMap2[ss.str()].bin, true, categoryMode );
      
      TString line = Form("%0.f-%0.f $\\otimes$ %.3f-%.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f & %.3f $\\pm$ %.3f \\\\",
			  tmp[0], tmp[2], tmp[1], tmp[3], totalsmh, totalUn, Nbkg, NbkgErr, Ns, NsErr);
      std::cout << line << std::endl;
    }
  
  std::cout << "\\hline\n\\end{tabular}\n\\end{center}\n\\end{table*}" << std::endl;

  TFile* sF = new TFile( "fullSys.root", "recreate" );
  nominal[0]->Write("SMH_nominal_0");
  nominal[1]->Write("SMH_nominal_1");
  nominal[2]->Write("SMH_nominal_2");
  nominal[3]->Write("SMH_nominal_3");
  facScaleUp->Write("facScaleUp");
  facScaleDown->Write("facScaleDown");
  renScaleUp->Write("renScaleUp");
  renScaleDown->Write("renScaleDown");
  facRenScaleUp->Write("facRenScaleUp");
  facRenScaleDown->Write("facRenScaleDown");
  JesUp->Write("JesUp");
  JesDown->Write("JesDown");
  btagUp->Write("btagUp");
  btagDown->Write("btagDown");
  misstagUp->Write("misstagUp");
  misstagDown->Write("misstagDown");
  for( int ipdf = 0; ipdf < 60; ipdf++ ) pdf[ipdf]->Write();
  sF->Close();
  
  return 0;
};

float GetNs( std::string fname, int bin, std::string cat )
{
  TFile* fin = TFile::Open( fname.c_str(), "READ");
  RooArgSet* norm_fit_s = (RooArgSet*) fin->Get("norm_fit_s");

  //return norm_fit_s->getRealValue("bin11/signal");
  std::stringstream ss;
  if ( cat == "highres" ) ss << "highResBin" << bin << "/signal";
  else if ( cat == "lowres" ) ss << "lowResBin" << bin << "/signal";
  else ss << "bin" << bin << "/signal";
  RooRealVar* ss2 = (RooRealVar*)norm_fit_s->find( ss.str().c_str() );
  return ss2->getVal();
};

float GetNsErr( std::string fname, int bin, std::string cat )
{
  TFile* fin = TFile::Open( fname.c_str(), "READ");
  RooArgSet* norm_fit_s = (RooArgSet*) fin->Get("norm_fit_s");
  //return norm_fit_s->getRealValue("bin11/signal");
  std::stringstream ss;
  if ( cat == "highres" ) ss << "highResBin" << bin << "/signal";
  else if ( cat == "lowres" ) ss << "lowResBin" << bin << "/signal";
  else ss << "bin" << bin << "/signal";
  RooRealVar* ss2 = (RooRealVar*)norm_fit_s->find( ss.str().c_str() );
  return ss2->getError();
};

float GetNbkg( std::string fname, std::string f1, int bin, bool _err, std::string cat )
{
  RooFit::PrintLevel(5);
  RooMsgService::instance().setGlobalKillBelow(RooFit::FATAL);
  TFile* fin = TFile::Open( fname.c_str(), "READ");
  RooFitResult* fit_r = (RooFitResult*)fin->Get("fit_s");
  /*fin = TFile::Open( "/Users/cmorgoth/Work/git/RazorEWKSUSYAnalysis/HggRazor/FitMgg/MaxLikelihoodFits/sb300_lsp1_unblinded_6p3ifb_Fixed/HggRazorWorkspace_bin8.root", "READ");
  RooWorkspace* myws = (RooWorkspace*)fin->Get("combineWS");
  RooRealVar* mymgg = (RooRealVar*)myws->var("mGammaGamma_bin8");
  RooAbsPdf* mypdf = (RooAbsPdf*)myws->pdf("singleExp_Bkg_bin8_sExp");
  myws->var("singleExp_Bkg_bin8_sExp_a")->setVal(-2.0071e-02);
  mymgg->setRange( "signal", 120, 130. );
  RooAbsReal* myigx = mypdf->createIntegral(*mymgg, RooFit::NormSet(*mymgg), RooFit::Range("signal"));;
  std::cout << "test--> " << myigx->getVal() << std::endl;
  myws = (RooWorkspace*)fin->Get("w_sb");
  RooFitResult* FR = (RooFitResult*)myws->obj("BkgOnlyFitResult");
  */
  TFile* ftmp = new TFile( "ws_tmp.root", "recreate");
  
 
  RooWorkspace* ws = new RooWorkspace( "ws", "" );
  TString mggName = "mGammaGamma";
  RooRealVar mgg( mggName, "m_{#gamma#gamma}", 103, 160, "GeV" );
  //mgg.SetNameTitle( mggName, "m_{#gamma#gamma}" );
  mgg.setMin( 103. );
  mgg.setMax( 160. );
  mgg.setUnit( "GeV" );
  mgg.setBins(57);
  mgg.setRange( "signal", 122, 129. );
  mgg.setRange( "high", 135., 160. );
  mgg.setRange( "low", 103., 120. );
  mgg.setRange( "full", 103., 160. );
  mgg.setRange( "Full", 103., 160. );
  if ( f1 == "singleExp" )
    {
      int realBin = bin;
      if ( cat == "lowres" ) realBin = realBin+5;
      std::stringstream ss;
      ss << "singleExp_Bkg_bin" << realBin << "_sExp_a";
      RooRealVar *alpha = (RooRealVar*)fit_r->floatParsFinal().find( ss.str().c_str() );
      
      std::stringstream ss_2;
      if ( cat == "highres" ) ss_2 << "shapeBkg_Bkg_highResBin" << bin << "__norm";
      else if ( cat == "lowres" ) ss_2 << "shapeBkg_Bkg_lowResBin" << bin << "__norm";
      else ss_2 << "shapeBkg_Bkg_bin" << bin << "__norm";
      RooRealVar *Nbkg = (RooRealVar*)fit_r->floatParsFinal().find( ss_2.str().c_str() );
      
      
      TString pdf = MakeSingleExpNE( f1, mgg, *ws );
      ws->var( pdf + "_a" )->setVal( alpha->getVal() );
      RooAbsReal* igx = ws->pdf( pdf )->createIntegral(mgg);
      //std::cout << Nbkg->getVal() << " +/- " << Nbkg->getError() << std::endl;
      RooAbsReal* igx_sig = ws->pdf( pdf )->createIntegral(mgg, RooFit::NormSet(mgg), RooFit::Range("signal"));
      //std::cout << Nbkg->getVal()*igx_sig->getVal() << " +/- " << Nbkg->getError()*igx_sig->getVal() << std::endl;
      if ( _err ) return Nbkg->getError()*igx_sig->getVal();
      return Nbkg->getVal()*igx_sig->getVal();
      //double shapeErr = ws->pdf( pdf )->getPropagatedError(*fit_r)*Nbkg->getVal();
      /*
      RooArgSet* paramSet = mypdf->getParameters( RooArgSet(*mymgg) );
      paramSet->Print();
      TF1* myPdf = mypdf->asTF( RooArgList(*mymgg), RooArgList(*paramSet) );
      TMatrixDSym covMatrix = FR->covarianceMatrix();
      double params[1];
      params[0] = myPdf->GetParameter(0);
      double corrP[1];
      corrP[0] = 8.15e-03;
      
      double integ2_full = myPdf->Integral(103, 160);
      double integ2_signal = myPdf->Integral(120, 130);
      //double shapeErr = mypdf->getPropagatedError(*fit_r)*Nbkg->getVal();
      //double intErr = myPdf->IntegralError( 120., 130., params, covMatrix.GetMatrixArray(), 1e-14 );
      std::cout << params[0] << std::endl;
      double intErr = myPdf->IntegralError( 120., 130., params, corrP );
      std::cout << integ2_signal << " " << integ2_full << " " << intErr << std::endl;
      //return alpha->getVal();
      myPdf->Write();
      ftmp->Close();
      */
    }
  else if ( f1 == "poly2" )
    {
      int realBin = bin;
      if ( cat == "lowres" ) realBin = realBin+5;
      std::stringstream ss0;
      ss0 << "poly2_Bkg_bin" << realBin << "_pol2_p0";
      RooRealVar *p0 = (RooRealVar*)fit_r->floatParsFinal().find( ss0.str().c_str() );
      std::stringstream ss1;
      ss1 << "poly2_Bkg_bin" << realBin << "_pol2_p1";
      RooRealVar *p1 = (RooRealVar*)fit_r->floatParsFinal().find( ss1.str().c_str() );
      std::stringstream ssC;
      ssC << "poly2_Bkg_bin" << realBin << "_pol2_pC";
      RooRealVar *pC = (RooRealVar*)fit_r->floatParsFinal().find( ssC.str().c_str() );
      
      std::stringstream ss_2;
      if ( cat == "highres" ) ss_2 << "shapeBkg_Bkg_highResBin" << bin << "__norm";
      else if ( cat == "lowres" ) ss_2 << "shapeBkg_Bkg_lowResBin" << bin << "__norm";
      else ss_2 << "shapeBkg_Bkg_bin" << bin << "__norm";
      RooRealVar *Nbkg = (RooRealVar*)fit_r->floatParsFinal().find( ss_2.str().c_str() );
      
      
      TString pdf = MakePoly2NE( f1, mgg, *ws );
      ws->var( pdf + "_p0" )->setVal( p0->getVal() );
      ws->var( pdf + "_p1" )->setVal( p1->getVal() );
      ws->var( pdf + "_pC" )->setVal( pC->getVal() );
      RooAbsReal* igx = ws->pdf( pdf )->createIntegral(mgg);
      //std::cout << Nbkg->getVal() << " +/- " << Nbkg->getError() << std::endl;
      RooAbsReal* igx_sig = ws->pdf( pdf )->createIntegral(mgg, RooFit::NormSet(mgg), RooFit::Range("signal"));
      //std::cout << Nbkg->getVal()*igx_sig->getVal() << " +/- " << Nbkg->getError()*igx_sig->getVal() << std::endl;
      if ( _err ) return Nbkg->getError()*igx_sig->getVal();
      return Nbkg->getVal()*igx_sig->getVal();
    }
  else if ( f1 == "poly3" )
    {
      int realBin = bin;
      if ( cat == "lowres" ) realBin = realBin+5;
      std::stringstream ss0;
      ss0 << "poly3_Bkg_bin" << realBin << "_pol3_p0";
      RooRealVar *p0 = (RooRealVar*)fit_r->floatParsFinal().find( ss0.str().c_str() );
      std::stringstream ss1;
      ss1 << "poly3_Bkg_bin" << realBin << "_pol3_p1";
      RooRealVar *p1 = (RooRealVar*)fit_r->floatParsFinal().find( ss1.str().c_str() );
      std::stringstream ss2;
      ss2 << "poly3_Bkg_bin" << realBin << "_pol3_p2";
      RooRealVar *p2 = (RooRealVar*)fit_r->floatParsFinal().find( ss2.str().c_str() );
      std::stringstream ssC;
      ssC << "poly3_Bkg_bin" << realBin << "_pol3_pC";
      RooRealVar *pC = (RooRealVar*)fit_r->floatParsFinal().find( ssC.str().c_str() );
      //std::cout << pC->getVal() << std::endl;

      std::stringstream ss_2;
      if ( cat == "highres" ) ss_2 << "shapeBkg_Bkg_highResBin" << bin << "__norm";
      else if ( cat == "lowres" ) ss_2 << "shapeBkg_Bkg_lowResBin" << bin << "__norm";
      else ss_2 << "shapeBkg_Bkg_bin" << bin << "__norm";
      RooRealVar *Nbkg = (RooRealVar*)fit_r->floatParsFinal().find( ss_2.str().c_str() );

      
      TString pdf = MakePoly3NE( f1, mgg, *ws );
      ws->var( pdf + "_p0" )->setVal( p0->getVal() );
      ws->var( pdf + "_p1" )->setVal( p1->getVal() );
      ws->var( pdf + "_p2" )->setVal( p1->getVal() );
      ws->var( pdf + "_pC" )->setVal( pC->getVal() );
      RooAbsReal* igx = ws->pdf( pdf )->createIntegral(mgg);
      //std::cout << Nbkg->getVal() << " +/- " << Nbkg->getError() << std::endl;
      RooAbsReal* igx_sig = ws->pdf( pdf )->createIntegral(mgg, RooFit::NormSet(mgg), RooFit::Range("signal"));
      //std::cout << Nbkg->getVal()*igx_sig->getVal() << " +/- " << Nbkg->getError()*igx_sig->getVal() << std::endl;
      if ( _err ) return Nbkg->getError()*igx_sig->getVal();
      return Nbkg->getVal()*igx_sig->getVal();
    }
  else if ( f1 == "modExp" )
    {
      int realBin = bin;
      if ( cat == "lowres" ) realBin = realBin+5;
      std::stringstream ss;
      ss << "modExp_Bkg_bin" << realBin << "_mexp_a";
      RooRealVar *alpha = (RooRealVar*)fit_r->floatParsFinal().find( ss.str().c_str() );
      std::stringstream ssm;
      ssm << "modExp_Bkg_bin" << realBin << "_mexp_m";
      RooRealVar *m = (RooRealVar*)fit_r->floatParsFinal().find( ssm.str().c_str() );
      
      std::stringstream ss_2;
      if ( cat == "highres" ) ss_2 << "shapeBkg_Bkg_highResBin" << bin << "__norm";
      else if ( cat == "lowres" ) ss_2 << "shapeBkg_Bkg_lowResBin" << bin << "__norm";
      else ss_2 << "shapeBkg_Bkg_bin" << bin << "__norm";
      RooRealVar *Nbkg = (RooRealVar*)fit_r->floatParsFinal().find( ss_2.str().c_str() );
      
      
      TString pdf = MakeModExpNE( f1, mgg, *ws );
      ws->var( pdf + "_a" )->setVal( alpha->getVal() );
      ws->var( pdf + "_m" )->setVal( m->getVal() );
      RooAbsReal* igx = ws->pdf( pdf )->createIntegral(mgg);
      //std::cout << Nbkg->getVal() << " +/- " << Nbkg->getError() << std::endl;
      RooAbsReal* igx_sig = ws->pdf( pdf )->createIntegral(mgg, RooFit::NormSet(mgg), RooFit::Range("signal"));
      //std::cout << Nbkg->getVal()*igx_sig->getVal() << " +/- " << Nbkg->getError()*igx_sig->getVal() << std::endl;
      if ( _err ) return Nbkg->getError()*igx_sig->getVal();
      return Nbkg->getVal()*igx_sig->getVal();
    }

  return 0;
};
