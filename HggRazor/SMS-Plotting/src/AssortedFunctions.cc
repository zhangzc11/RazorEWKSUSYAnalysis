#include <iostream>
//ROOT
#include <TFile.h>
#include <TTree.h>
//LOCAL
#include "AssortedFunctions.hh"

std::map<float, float> xsecMap;
 
void GetLimit(Limit &limit, std::string fname, float ssFactor )
{
  TFile* f = new TFile( fname.c_str(), "READ" );
  TTree* tree = (TTree*)f->Get("limit");
  double _limit;
  tree->SetBranchAddress( "limit", &_limit );
  //----------------------------
  //Retrieving limit information
  //----------------------------
  tree->GetEntry(0);
  limit.q2p5 = _limit*ssFactor;
  tree->GetEntry(1);
  limit.q16 = _limit*ssFactor;
  tree->GetEntry(2);
  limit.exp = _limit*ssFactor;
  tree->GetEntry(3);
  limit.q84 = _limit*ssFactor;
  tree->GetEntry(4);
  limit.q97p5 = _limit*ssFactor;
  tree->GetEntry(5);
  limit.obs = _limit*ssFactor;
  std::pair<float, float> massPair = GetMasses( fname );
  limit.msb  = massPair.first;
  limit.mlsp = massPair.second;
  limit.xsecL = GetCrossSectionLimit( limit.obs, limit.msb);
};
std::pair<float, float> GetMasses(std::string fname)
{
  int low = fname.find("mSbottom-") + 9;
  int high = fname.find("_mLSP") - low;
  std::string sbm = fname.substr(low,high);
  low = fname.find("mLSP-") + 5;
  high = fname.find("_Asymptotic") - low;
  std::string lspm = fname.substr(low,high);
  //std::cout << "msb,lsp: " << sbm << "," << lspm << std::endl;
  return std::make_pair( atof(sbm.c_str()), atof(lspm.c_str()) );
};

void FillCrossSectionMap( std::string xsecfname )
{
  std::ifstream ifs( xsecfname.c_str(), std::fstream::in );
  if ( ifs.is_open() )
    {
      std::string mass, xsec;
      while ( ifs.good() )
	{
	  ifs >> mass >> xsec;
	  if ( ifs.eof() ) break;
	  if ( mass.find("#") != std::string::npos ) continue;
	  float _mass = atof(mass.c_str());
	  if ( xsecMap.find(_mass) == xsecMap.end() ) xsecMap[_mass] = atof( xsec.c_str() ); 
	  //std::cout << "[INFO]: mass - xsec " << mass << "-" << strtof(xsec.c_str(),NULL) << std::endl;
	  //std::cout << "[INFO]: mass - xsec " << _mass << "-" << xsecMap[_mass] << std::endl;
	}
    }
  else
    {
      std::cout << "Unable to open binning lookup table" << std::endl;
    }
};

float GetCrossSectionLimit( float ssLimit, float msb )
{
  if ( xsecMap.size() == 0 )
    {
      std::cerr << "[ERROR]: xsec map is empty; make sure you initialized the xsecMap using FillCrossSectionMap( std::string xsecfname )!" << std::endl;
      return -1;
    }

  if ( xsecMap.find(msb) == xsecMap.end() )
    {
      std::cerr << "[ERROR]: xsec for mass " << msb <<" was not found; please check your xsec input file" << std::endl;
      return -1;
    }
  
  return xsecMap[msb]*ssLimit;
};
