

void MakeSignificancePlot( int option = 1) {

  int NBins = 20;
  Double_t x[NBins];  
  Double_t y[NBins];
  x[0] = -0.5;
  for (int i=1; i<NBins; ++i)  x[i] = i-1;


  if (option == 0) {
    //use significance computed from log likelihood
    
    //highPt Category
    y[0] = -1000;
    y[1] = 1.1;
    y[2] = 0.9;
    y[3] = 0;
    y[4] = -0.7;
    y[5] = 0.8;
    y[6] = 1.3;
    y[7] = 0.8;
    y[8] = 0.7;
    //hbb category  
    y[9] = 0.6;
    //highres category
    y[10] = -0.4;
    y[11] = 1.1;
    y[12] = 0.8;
    y[13] = 1.7;
    y[14] = 0.7;
    //low res category
    y[15] = 0.0;
    y[16] = 1.1;
    y[17] = 0.8;
    y[18] = 0.8;
    y[19] = 0.7;
  } else if (option == 1) {
    //use fitted signal yield / uncertainty
    
    //highPt Category
    y[0] = -1000;
    y[1] = 2.0/2.2;
    y[2] = 2.0/2.5;
    y[3] = 0;
    y[4] = -8.6/11.4;
    y[5] = 3.9/5.3;
    y[6] = 4.3/3.5;
    y[7] = 3.9/4.8;
    y[8] = 1.9/2.7;
    //hbb category  
    y[9] = 1.9/3.3;
    //highres category
    y[10] = -8.8/20.3;
    y[11] = 2.7/2.3;
    y[12] = 2.7/3.7;
    y[13] = 21.3/12.2;
    y[14] = 3.5/4.4;
    //low res category
    y[15] = -3.6/8.6;
    y[16] = 0.5/0.5;
    y[17] = 0.9/1.3;
    y[18] = 7.2/4.6;
    y[19] = 1.4/1.9;
  }


  TCanvas *cv = new TCanvas("cv","cv",800,600);
  TH1F *hist = 0;
  if (option == 0) hist = new TH1F("hist",";Bin Number; Observed Significance;", 19, -0.5, 18.5);
  else if (option == 1) hist = new TH1F("hist",";Bin Number; Fitted Signal Yield / Uncertainty;", 19, -0.5, 18.5);
  // else if (option == 1) hist = new TH1F("hist",";Bin Number; Fitted N_{signal} / #sigma_{N_{signal}};", 19, -0.5, 18.5);
  
  hist->SetStats(0);
  hist->SetMaximum(5);
  hist->SetMinimum(-7);
  hist->GetYaxis()->SetTitleSize(0.05);
  hist->GetYaxis()->SetTitleOffset(0.85);
  hist->GetYaxis()->SetLabelSize(0.045);
  hist->GetXaxis()->SetTitleSize(0.05);
  hist->GetXaxis()->SetTitleOffset(0.85);
  hist->GetXaxis()->SetLabelSize(0.045);
  hist->Draw();


  TGraph *gr =0;
  gr = new TGraphAsymmErrors(NBins,x,y);
  gr->SetTitle("");
  //gr->Draw("psame");
  gr->SetLineWidth(2);
  gr->SetMarkerStyle(20);
  gr->SetMarkerSize(1);
  // gr->GetXaxis()->SetTitle("Bin Number");
  // gr->GetYaxis()->SetTitle("Observed Significance");
  // gr->GetXaxis()->SetTitleSize(0.05);
  // gr->GetXaxis()->SetTitleOffset(0.8);
  // gr->GetYaxis()->SetTitleSize(0.05);
  // gr->GetYaxis()->SetTitleOffset(0.8);
  // gr->GetYaxis()->SetRangeUser(-7,5);

  TBox *TwoSigmaBand = new TBox(-0.48, -2, 18.5, 2);
  TwoSigmaBand->SetFillColor(kGreen);
  //box->SetFillStyle(3001);
  TwoSigmaBand->Draw("same");

  TBox *OneSigmaBand = new TBox(-0.48, -1, 18.5, 1);
  OneSigmaBand->SetFillColor(kYellow);
  //box->SetFillStyle(3001);
  OneSigmaBand->Draw("same");

  TLine *l = new TLine(-0.48,0,18.5,0);
  l->SetLineWidth(2);
  l->SetLineColor(kBlue);
  l->Draw();

  gr->Draw("P");

  TLatex *tex = new TLatex();
  tex->SetNDC();
  tex->SetTextSize(0.040);
  tex->SetTextFont(42);
  tex->SetTextColor(kBlack);
  tex->DrawLatex(0.7, 0.92, "6.3 fb^{-1} (13 TeV)");
  tex->DrawLatex(0.4, 0.92, "Razor H#rightarrow#gamma#gamma Search");
  tex->Draw();
  
  TLatex *CMSLabel = new TLatex();
  CMSLabel->SetNDC();
  CMSLabel->SetTextSize(0.050);
  CMSLabel->SetTextFont(42);
  CMSLabel->SetTextColor(kBlack);
  CMSLabel->DrawLatex(0.1,0.92,"#bf{CMS}");
  CMSLabel->SetTextSize(0.045);
  CMSLabel->DrawLatex(0.19,0.92,"Preliminary");
  CMSLabel->Draw();


  TLine *l1 = new TLine(7.5,-7,7.5,5);
  l1->SetLineWidth(2);
  l1->SetLineStyle(2);
  l1->SetLineColor(kBlack);
  l1->Draw();

  TLine *l2 = new TLine(8.5,-7,8.5,5);
  l2->SetLineWidth(2);
  l2->SetLineStyle(2);
  l2->SetLineColor(kBlack);
  l2->Draw();

   TLine *l3 = new TLine(13.5,-7,13.5,5);
  l3->SetLineWidth(2);
  l3->SetLineStyle(2);
  l3->SetLineColor(kBlack);
  l3->Draw();

  TLatex *HighPtLabel = new TLatex();
  HighPtLabel->SetNDC();
  HighPtLabel->SetTextAngle(90);
  HighPtLabel->SetTextSize(0.035);
  HighPtLabel->SetTextFont(42);
  HighPtLabel->SetTextColor(kBlack);
  HighPtLabel->DrawLatex(0.32,0.13,"#bf{HighPt Category}");
  HighPtLabel->Draw();

  TLatex *HbbLabel = new TLatex();
  HbbLabel->SetNDC();
  HbbLabel->SetTextAngle(90);
  HbbLabel->SetTextSize(0.035);
  HbbLabel->SetTextFont(42);
  HbbLabel->SetTextColor(kBlack);
  HbbLabel->DrawLatex(0.465,0.13,"#bf{HZbb Category}");
  HbbLabel->Draw();


   TLatex *HighResLabel = new TLatex();
  HighResLabel->SetNDC();
  HighResLabel->SetTextAngle(90);
  HighResLabel->SetTextSize(0.035);
  HighResLabel->SetTextFont(42);
  HighResLabel->SetTextColor(kBlack);
  HighResLabel->DrawLatex(0.60,0.13,"#bf{HighRes Category}");
  HighResLabel->Draw();

   TLatex *LowResLabel = new TLatex();
  LowResLabel->SetNDC();
  LowResLabel->SetTextAngle(90);
  LowResLabel->SetTextSize(0.035);
  LowResLabel->SetTextFont(42);
  LowResLabel->SetTextColor(kBlack);
  LowResLabel->DrawLatex(0.80,0.13,"#bf{LowRes Category}");
  LowResLabel->Draw();

 

  cv->SaveAs("SignificanceVsBin.png");
  cv->SaveAs("SignificanceVsBin.pdf");


}
