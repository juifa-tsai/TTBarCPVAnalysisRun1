#include <fstream>
#include <iostream>
#include <vector>
#include "TH2.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TMinuit.h"
#include "TMath.h"
#include "TVirtualFitter.h"
#include "TFile.h"
#include "TLegend.h"
#include "TMarker.h"
#include "TLine.h"
#include "TRandom2.h"
#include "TPaveText.h"
#define NPAR 2
#define SIGFRAC 0.8 // Default is 0.5
using namespace std;

vector<Double_t> dataColl;
vector<Double_t> sigColl;
vector<Double_t> bkgColl;

void fcn( Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag )
{

  Double_t Lsum=0.;
  Double_t Nevt=0.;
  Double_t fs = par[0];
  Double_t fb = par[1];
  for ( int i=0; i<int(dataColl.size()); i++ ) {
    Nevt += dataColl[i];
    //PDF for signal and background
    Double_t Ls = sigColl[i];
    Double_t Lb = bkgColl[i];
    for (int data=0; data<int(dataColl[i]); data++) {
      //Get Log Likelihood
      if( Ls!=0. || Lb!=0.) Lsum += TMath::Log( (fs*Ls + fb*Lb) / (fs+fb) );
      //if( Ls!=0. && Lb!=0. ) Lsum += TMath::Log( (fs*Ls + fb*Lb) / (fs+fb) );
    }
  }
  f=2*( -1*Lsum + (fs+fb)  - Nevt*TMath::Log(fs+fb) );
}

Double_t* fitter_LH( TH1D* hSig, TH1D* hBkg, TH1D* hData )
{
    Double_t *fitted = new Double_t[8];
    int nData = hData->GetEntries();
    int nbins = hData->GetNbinsX();

    dataColl.clear();
    sigColl.clear();
    bkgColl.clear();

    // normalize template
    printf(" --------- before the fit ------------- \n");
    printf("Nsig %2.3f, Nbg %2.3f \n", hSig->Integral(), hBkg->Integral());
    printf("Purity %2.3f, init size %4.3f,  test sample size %4d\n", hSig->Integral()/(hSig->Integral()+hBkg->Integral()), (hSig->Integral()+hBkg->Integral()), nData);
    printf(" -------------------------------------- \n");

    hSig->Scale(1./hSig->Integral());
    hBkg->Scale(1./hBkg->Integral());  

    for (int ibin=1; ibin<=nbins; ibin++) {
        dataColl.push_back(hData->GetBinContent(ibin));
        sigColl.push_back(hSig->GetBinContent(ibin));
        bkgColl.push_back(hBkg->GetBinContent(ibin));    
    }

    printf( " -----  Got %d, %d, %d events for fit ----- \n ", int(dataColl.size()), int(sigColl.size()), int(bkgColl.size()) );  
    if ( dataColl.size() != sigColl.size() || sigColl.size()!=bkgColl.size() ) {
        printf(" error ...  inconsistent hit collection size \n");
        return fitted;
    }

    //init parameters for fit
    Int_t ierflg = 0;
    Double_t step[]={0.1, 0.1};
    Double_t vstart[10]={1., 1.};
    Double_t arglist[10];
    vstart[0] = SIGFRAC*nData;
    vstart[1] = (1-SIGFRAC)*nData;

    TMinuit *gMinuit = new TMinuit(NPAR);  
    gMinuit->Command("SET STR 1");
    gMinuit->SetFCN(fcn);

    arglist[0] = 1;
    gMinuit->mnexcm( "SET ERR",   arglist, 1, ierflg );
    arglist[0] = 1;
    gMinuit->mnexcm( "SET PRINT", arglist, 1, ierflg );
    gMinuit->mnparm( 0, "Signal yield",     vstart[0], step[0], 0., nData*2., ierflg );
    gMinuit->mnparm( 1, "background yield", vstart[1], step[1], 0., nData*2., ierflg );

    printf(" --------------------------------------------------------- \n");
    printf(" Now ready for minimization step \n --------------------------------------------------------- \n");

    arglist[0] = 2000; // number of iteration
    arglist[1] = 1.;
    gMinuit->mnexcm("MIGRAD", arglist ,2,ierflg);
    printf (" -------------------------------------------- \n");
    printf("Finished.  ierr = %2.2d \n", ierflg);

    double para[NPAR+1], errpara[NPAR+1];
    if ( ierflg == 0 ) 
    {
        for(int j=0; j<=NPAR-1;j++) {
            gMinuit->GetParameter(j, para[j],errpara[j]);
            para[NPAR] = dataColl.size();
            printf("Parameter (yeild) %d = %f +- %f\n",j,para[j],errpara[j]);
        }
        printf(" fitted yield %2.3f \n", (para[0]+para[1])/nData );
    }
    else {
        printf(" *********** Fit failed! ************\n");
        gMinuit->GetParameter(0, para[0],errpara[0]);
        gMinuit->GetParameter(1, para[1],errpara[1]);
        para[0]=0.; errpara[0]=0.;
    }
    // Print results
    Double_t amin,edm,errdef;
    Int_t nvpar,nparx,icstat;
    gMinuit->mnstat(amin,edm,errdef,nvpar,nparx,icstat);
    gMinuit->mnprin(1,amin);  
    gMinuit->mnmatu(1);

    printf(" ========= happy ending !? =========================== \n");
    printf("FCN =  %3.3f \n", amin);
    printf("Fitted sig =  %3.3f (%3.3f)\n", para[0], errpara[0]);
    printf("Fitted bkg =  %3.3f (%3.3f)\n", para[1], errpara[1]);

    hSig->Scale(para[0]);
    hBkg->Scale(para[1]);

    fitted[0]=para[0];    //sig yields
    fitted[1]=errpara[0]; //sig err
    fitted[2]=para[1];    //bkg yields
    fitted[3]=errpara[1]; //bkg err

    return fitted;
}

Double_t* fitter_LH_Plot( TH1D* hSig_, TH1D* hBkg_, TH1D* hData_, std::string name, int chN=0, std::string output=".", std::string xTitle="", std::string yTitle="Events" )
//Double_t* fitter_LH_Plot( TH1D* hSig, TH1D* hBkg, TH1D* hData_, std::string name, int chN=0, std::string output=".", std::string xTitle="", std::string yTitle="Events" )
{
    double count=0;
    dataColl.clear();
    sigColl.clear();
    bkgColl.clear();

    //Get data from looping tree
    char hname[30];
    std::string ch;
    std::string channel;
    if( chN == 1 ){
        ch="_El";
        channel="Electron channel";
    }else if( chN == 2){
        ch="_Mu";
        channel="Muon channel";
    }else{
        ch="";
        channel="Combined channel";
    }

    //Double_t* fitted = fitter_LH( hSig, hBkg, hData_ );
    Double_t* fitted = fitter_LH( hSig_, hBkg_, hData_ );
    TH1D* hData = (TH1D*)hData_->Clone();
    TH1D* hSig  = (TH1D*)hSig_->Clone();
    TH1D* hBkg  = (TH1D*)hBkg_->Clone();

    TH1D* hFitted = (TH1D*)hBkg->Clone();
    hFitted->Add(hSig);

    // plot
    double yerr[100];
    for(int i=0;i<100;i++){ yerr[i] = 0.; }
    hFitted->SetError(yerr);
    hBkg->SetError(yerr);
    hSig->SetError(yerr);

    TCanvas *c1 = new TCanvas("HF1", "Histos1", 258,92,748,702);
    c1->Range(-104.4905,-2560.33,537.9965,11563.46);
    c1->SetFillColor(0);
    c1->SetBorderMode(0);
    c1->SetBorderSize(2);
    c1->SetLeftMargin(0.1626344);
    c1->SetRightMargin(0.05913978);
    c1->SetTopMargin(0.05349183);
    c1->SetBottomMargin(0.1812779);
    c1->SetFrameBorderMode(0);
    c1->SetFrameBorderMode(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0); 
    c1->Draw(); 

    hSig->SetLineColor(4);
    hSig->SetLineWidth(2);

    hData->SetLineColor(1);
    hData->SetXTitle(xTitle.c_str());
    hData->SetYTitle(yTitle.c_str());
    hData->SetTitle("");
    hData->SetMarkerStyle(8);
    hData->SetMinimum(0.);
    hData->GetXaxis()->SetNdivisions(505);
    hData->GetXaxis()->SetLabelFont(42);
    hData->GetXaxis()->SetLabelSize(0.05);
    hData->GetXaxis()->SetTitleSize(0.06);
    hData->GetXaxis()->SetTitleOffset(1.15);
    hData->GetXaxis()->SetTitleFont(42);
    hData->GetYaxis()->SetNdivisions(505);
    hData->GetYaxis()->SetLabelFont(42);
    hData->GetYaxis()->SetLabelSize(0.035);
    hData->GetYaxis()->SetTitleSize(0.06);
    hData->GetYaxis()->SetTitleOffset(1.21);
    hData->GetYaxis()->SetTitleFont(42);

    float ymax = hData->GetMaximum();
    if ( hFitted->GetMaximum() > hData->GetMaximum() ) ymax = hFitted->GetMaximum();
    if ( hData->GetMaximum() < 15 ) ymax = 15;
    hData->SetMaximum(ymax*1.4);
    hBkg->SetMarkerStyle(0);
    hBkg->SetFillColor(2);
    hBkg->SetLineWidth(1);
    hBkg->SetLineColor(2);
    hBkg->SetFillStyle(3005);
    hSig->SetMarkerStyle(0);
    hSig->SetLineStyle(2);

    hFitted->SetMarkerStyle(0);
    hFitted->SetLineColor(1);
    hFitted->SetLineWidth(2);

    hData->Draw("e p");
    hBkg->Draw("h same");
    hSig->Draw("h same");
    hFitted->Draw("h same");
    hData->Draw("e p same");

    TLegend *tleg = new TLegend(0.5241935,0.6344725,0.8682796,0.9331352,NULL,"brNDC");
    char text[50];
    tleg->SetHeader(channel.c_str());
    tleg->SetBorderSize(0);
    tleg->SetTextSize(0.03120357);
    tleg->SetLineColor(1);
    tleg->SetLineStyle(1);
    tleg->SetLineWidth(1);
    tleg->SetFillColor(0);
    tleg->SetFillStyle(0);
    cout<<"[ "<<channel<<" ]"<<endl;
    sprintf( text, "Data %5.1f events",  hData->Integral()); cout<<text<<endl;
    tleg->AddEntry( hData, text,"elp");
    sprintf( text, "Fitted %5.1f events",hFitted->Integral()); cout<<text<<endl;
    tleg->AddEntry( hFitted, text,"l");
    sprintf( text, "SIG %5.1f #pm %5.1f events", fitted[0], fitted[1] ); cout<<text<<endl;
    tleg->AddEntry( hSig, text, "f");
    sprintf( text, "BKG %5.1f #pm %5.1f events", fitted[2], fitted[3] ); cout<<text<<endl;
    tleg->AddEntry( hBkg, text, "f");
    tleg->Draw();
    
    TPaveText* t_title;
    t_title = new TPaveText(0.1317204,0.9451852,0.7620968,0.9896296,"brNDC");
    t_title->AddText("CMS #sqrt{s} = 8TeV, L=19.7 fb^{-1}");
    t_title->SetTextColor(kBlack);
    t_title->SetFillColor(kWhite);
    t_title->SetFillStyle(0);
    t_title->SetBorderSize(0);
    t_title->SetTextAlign(12);
    t_title->SetTextSize(0.04);

    t_title->Draw();

    hData->Chi2Test( hFitted, "P");
    c1->SaveAs((output+"/FittingResults_"+name+ch+".pdf").c_str());

    delete hData; 
    delete hSig; 
    delete hBkg; 
    delete hFitted;
    return fitted;
}
Double_t* fitter_LH_Plot( TFile* fin, std::string name, int chN=0, std::string output=".", std::string xTitle="", std::string yTitle="Events" )
{
    double count=0;
    dataColl.clear();
    sigColl.clear();
    bkgColl.clear();

    //Get data from looping tree
    char hname[30];
    std::string ch;
    std::string channel;
    if( chN == 1 ){
        ch="_El";
        channel="Electron channel";
    }else if( chN == 2){
        ch="_Mu";
        channel="Muon channel";
    }else{
        ch="";
        channel="Combined channel";
    }

    TH1D * hSig  = (TH1D*)((TH1D*)fin->Get(("SigMC"+ch).c_str()))->Clone();
    TH1D * hBkg  = (TH1D*)((TH1D*)fin->Get(("BkgMC"+ch).c_str()))->Clone();
    TH1D * hData = (TH1D*)((TH1D*)fin->Get(("DATA"+ch).c_str()))->Clone();
    TH1D * hFitted = new TH1D();

    Double_t* fitted = fitter_LH( hSig, hBkg, hData );

    hFitted = (TH1D*)hBkg->Clone();
    hFitted->Add(hSig);

    // plot
    double yerr[100];
    for(int i=0;i<100;i++){ yerr[i] = 0.; }
    hFitted->SetError(yerr);
    hBkg->SetError(yerr);
    hSig->SetError(yerr);

    TCanvas *c1 = new TCanvas("HF1", "Histos1", 258,92,748,702);
    c1->Range(-104.4905,-2560.33,537.9965,11563.46);
    c1->SetFillColor(0);
    c1->SetBorderMode(0);
    c1->SetBorderSize(2);
    c1->SetLeftMargin(0.1626344);
    c1->SetRightMargin(0.05913978);
    c1->SetTopMargin(0.05349183);
    c1->SetBottomMargin(0.1812779);
    c1->SetFrameBorderMode(0);
    c1->SetFrameBorderMode(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0); 
    c1->Draw(); 

    hSig->SetLineColor(4);
    hSig->SetLineWidth(2);

    hData->SetLineColor(1);
    hData->SetXTitle(xTitle.c_str());
    hData->SetYTitle(yTitle.c_str());
    hData->SetTitle("");
    hData->SetMarkerStyle(8);
    hData->SetMinimum(0.);
    hData->GetXaxis()->SetNdivisions(505);
    hData->GetXaxis()->SetLabelFont(42);
    hData->GetXaxis()->SetLabelSize(0.05);
    hData->GetXaxis()->SetTitleSize(0.06);
    hData->GetXaxis()->SetTitleOffset(1.15);
    hData->GetXaxis()->SetTitleFont(42);
    hData->GetYaxis()->SetNdivisions(505);
    hData->GetYaxis()->SetLabelFont(42);
    hData->GetYaxis()->SetLabelSize(0.035);
    hData->GetYaxis()->SetTitleSize(0.06);
    hData->GetYaxis()->SetTitleOffset(1.21);
    hData->GetYaxis()->SetTitleFont(42);

    float ymax = hData->GetMaximum();
    if ( hFitted->GetMaximum() > hData->GetMaximum() ) ymax = hFitted->GetMaximum();
    if ( hData->GetMaximum() < 15 ) ymax = 15;
    hData->SetMaximum(ymax*1.4);
    hBkg->SetMarkerStyle(0);
    hBkg->SetFillColor(2);
    hBkg->SetLineWidth(1);
    hBkg->SetLineColor(2);
    hBkg->SetFillStyle(3005);
    hSig->SetMarkerStyle(0);
    hSig->SetLineStyle(2);

    hFitted->SetMarkerStyle(0);
    hFitted->SetLineColor(1);
    hFitted->SetLineWidth(2);

    hData->Draw("e p");
    hBkg->Draw("h same");
    hSig->Draw("h same");
    hFitted->Draw("h same");
    hData->Draw("e p same");

    TLegend *tleg = new TLegend(0.5241935,0.6344725,0.8682796,0.9331352,NULL,"brNDC");
    char text[50];
    tleg->SetHeader(channel.c_str());
    tleg->SetBorderSize(0);
    tleg->SetTextSize(0.03120357);
    tleg->SetLineColor(1);
    tleg->SetLineStyle(1);
    tleg->SetLineWidth(1);
    tleg->SetFillColor(0);
    tleg->SetFillStyle(0);
    cout<<"[ "<<channel<<" ]"<<endl;
    sprintf( text, "Data %5.1f events",  hData->Integral()); cout<<text<<endl;
    tleg->AddEntry( hData, text,"elp");
    sprintf( text, "Fitted %5.1f events",hFitted->Integral()); cout<<text<<endl;
    tleg->AddEntry( hFitted, text,"l");
    sprintf( text, "SIG %5.1f #pm %5.1f events", fitted[0], fitted[1] ); cout<<text<<endl;
    tleg->AddEntry( hSig, text, "f");
    sprintf( text, "BKG %5.1f #pm %5.1f events", fitted[2], fitted[3] ); cout<<text<<endl;
    tleg->AddEntry( hBkg, text, "f");
    tleg->Draw();
    
    TPaveText* t_title;
    t_title = new TPaveText(0.1317204,0.9451852,0.7620968,0.9896296,"brNDC");
    t_title->AddText("CMS #sqrt{s} = 8TeV, L=19.7 fb^{-1}");
    t_title->SetTextColor(kBlack);
    t_title->SetFillColor(kWhite);
    t_title->SetFillStyle(0);
    t_title->SetBorderSize(0);
    t_title->SetTextAlign(12);
    t_title->SetTextSize(0.04);

    t_title->Draw();

    hData->Chi2Test( hFitted, "P");
    c1->SaveAs((output+"/FittingResults_"+name+ch+".pdf").c_str());

    return fitted;
}

void pullTest( TFile* fin, std::string name, int chN=0, int nExp=1000, float nSig=400, float nBkg=400, float eSig=0.4, float eBkg=0.4, std::string output="." )
{
    char hname[30];
    std::string ch;
    std::string channel;
    if( chN == 1 ){
        ch="_El";
        channel="Electron channel";
    }else if( chN == 2){
        ch="_Mu";
        channel="Muon channel";
    }else{
        ch="";
        channel="Combined channel";
    }

    float bins=  60;
    float min = -600;
    float max =  600;

    TH1D* hSig = (TH1D*)((TH1D*)fin->Get(("SigMC"+ch).c_str()))->Clone();
    TH1D* hBkg = (TH1D*)((TH1D*)fin->Get(("BkgMC"+ch).c_str()))->Clone();

    TFile* fout = new TFile((output+"/PullTest_"+name+ch+".root").c_str(), "RECREATE");
    TH1D* hPullSig = new TH1D("hPullSig", "", 30, -3, 3);
    TH1D* hPullBkg = new TH1D("hPullBkg", "", 30, -3, 3);
    TH1D* hResiSig = new TH1D("hResiSig", "", bins, min, max);
    TH1D* hResiBkg = new TH1D("hResiBkg", "", bins, min, max);

    int  nbins = hSig->GetNbinsX();
    float bMin = hSig->GetBinLowEdge(1);
    float bMax = hSig->GetBinLowEdge(nbins+1);

    for( int iExp=0; iExp<nExp; iExp++ )
    {
        cout<<"------------------------ Exp: "<<iExp+1<<" -----------------------"<<endl;

        TH1D* hSig_ = (TH1D*)hSig->Clone();
        TH1D* hBkg_ = (TH1D*)hBkg->Clone();
        TH1D *toySig = new TH1D("toySig", "", nbins, bMin, bMax); toySig->Sumw2();
        TH1D *toyBkg = new TH1D("toyBkg", "", nbins, bMin, bMax); toyBkg->Sumw2();

        toySig->FillRandom( hSig_, nSig ); 
        toyBkg->FillRandom( hBkg_, nBkg );

        TH1D* toySum = (TH1D*)toySig->Clone();
        toySum->Add(toyBkg);    

        Double_t* fitted = fitter_LH( hSig_, hBkg_, toySum );
        delete hSig_;
        delete hBkg_;
        delete toySum;
        delete toySig; 
        delete toyBkg; 

        float fittedSig = fitted[0];
        float fittedBkg = fitted[2];
        hPullSig->Fill((fittedSig-nSig)/eSig);
        hPullBkg->Fill((fittedBkg-nBkg)/eBkg);
        hResiSig->Fill((fittedSig-nSig));
        hResiBkg->Fill((fittedBkg-nBkg));
    }

    //hPullSig->SetTitle("Signal");
    //hPullBkg->SetTitle("Background");
    hResiSig->GetYaxis()->SetTitle("Number of exp.");
    hResiBkg->GetYaxis()->SetTitle("Number of exp.");
    hResiSig->GetXaxis()->SetTitle("Resisual");
    hResiBkg->GetXaxis()->SetTitle("Resisual");
    hResiSig->GetXaxis()->SetNdivisions(30504);
    hResiSig->GetXaxis()->SetLabelFont(42);
    hResiSig->GetXaxis()->SetLabelSize(0.05);
    hResiSig->GetXaxis()->SetTitleSize(0.07);
    hResiSig->GetXaxis()->SetTitleOffset(0.83);
    hResiSig->GetXaxis()->SetTitleFont(42);
    hResiSig->GetYaxis()->SetLabelFont(42);
    hResiSig->GetYaxis()->SetLabelSize(0.05);
    hResiSig->GetYaxis()->SetTitleSize(0.07);
    hResiSig->GetYaxis()->SetTitleOffset(0.97);
    hResiSig->GetYaxis()->SetTitleFont(42);
    hResiBkg->GetXaxis()->SetNdivisions(30504);
    hResiBkg->GetXaxis()->SetLabelFont(42);
    hResiBkg->GetXaxis()->SetLabelSize(0.05);
    hResiBkg->GetXaxis()->SetTitleSize(0.07);
    hResiBkg->GetXaxis()->SetTitleOffset(0.83);
    hResiBkg->GetXaxis()->SetTitleFont(42);
    hResiBkg->GetYaxis()->SetLabelFont(42);
    hResiBkg->GetYaxis()->SetLabelSize(0.05);
    hResiBkg->GetYaxis()->SetTitleSize(0.07);
    hResiBkg->GetYaxis()->SetTitleOffset(0.97);
    hResiBkg->GetYaxis()->SetTitleFont(42);

    hPullSig->GetYaxis()->SetTitle("Number of exp.");
    hPullBkg->GetYaxis()->SetTitle("Number of exp.");
    hPullSig->GetXaxis()->SetTitle("#sigma");
    hPullBkg->GetXaxis()->SetTitle("#sigma");
    hPullSig->GetXaxis()->SetNdivisions(30504);
    hPullSig->GetXaxis()->SetLabelFont(42);
    hPullSig->GetXaxis()->SetLabelSize(0.05);
    hPullSig->GetXaxis()->SetTitleSize(0.07);
    hPullSig->GetXaxis()->SetTitleOffset(0.83);
    hPullSig->GetXaxis()->SetTitleFont(42);
    hPullSig->GetYaxis()->SetLabelFont(42);
    hPullSig->GetYaxis()->SetLabelSize(0.05);
    hPullSig->GetYaxis()->SetTitleSize(0.07);
    hPullSig->GetYaxis()->SetTitleOffset(0.97);
    hPullSig->GetYaxis()->SetTitleFont(42);
    hPullBkg->GetXaxis()->SetNdivisions(30504);
    hPullBkg->GetXaxis()->SetLabelFont(42);
    hPullBkg->GetXaxis()->SetLabelSize(0.05);
    hPullBkg->GetXaxis()->SetTitleSize(0.07);
    hPullBkg->GetXaxis()->SetTitleOffset(0.83);
    hPullBkg->GetXaxis()->SetTitleFont(42);
    hPullBkg->GetYaxis()->SetLabelFont(42);
    hPullBkg->GetYaxis()->SetLabelSize(0.05);
    hPullBkg->GetYaxis()->SetTitleSize(0.07);
    hPullBkg->GetYaxis()->SetTitleOffset(0.97);
    hPullBkg->GetYaxis()->SetTitleFont(42);

    TCanvas *c1 = new TCanvas("c1_pull", "",1396,155,810,716);
    c1->Range(-865.285,-0.2176583,805.1813,1.166891);
    c1->SetFillColor(0);
    c1->SetBorderMode(0);
    c1->SetBorderSize(2);
    c1->SetLeftMargin(0.1588089);
    c1->SetRightMargin(0.1228288);
    c1->SetTopMargin(0.08442504);
    c1->SetBottomMargin(0.1572052);
    c1->SetFrameBorderMode(0);
    c1->SetFrameBorderMode(0);
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(101);
    gStyle->SetFitFormat("3.3g");

    char text[50];
    TPaveText *pt = new TPaveText(0.2543424,0.9330422,0.5148883,0.9898108,"blNDC");
    pt->SetBorderSize(0);
    pt->SetFillColor(0);
    pt->SetFillStyle(0);
    pt->SetTextFont(42);
    pt->SetTextSize(0.04366812);
    TF1* gausResi1 = new TF1("gausResi1", "gaus", min, max);
    TF1* gausResi2 = new TF1("gausResi2", "gaus", min, max);
    TF1* gausPull1 = new TF1("gausPull1", "gaus", min, max);
    TF1* gausPull2 = new TF1("gausPull2", "gaus", min, max);
    gausResi1->SetLineColor(2);
    gausResi2->SetLineColor(2);
    gausPull1->SetLineColor(2);
    gausPull2->SetLineColor(2);

    sprintf( text, "Expected sig. %.0f (%d exp.) ", nSig, nExp ); 
    hPullSig->Fit( gausPull1, "WR"); cout<<endl;
    hPullSig->Draw();
    gausPull1->Draw("SAME");
    pt->AddText(text);
    pt->Draw();
    c1->SaveAs((output+"/PullTestSig_"+name+ch+".pdf").c_str());

    hResiSig->Fit( gausResi1, "WR"); cout<<endl;
    hResiSig->Draw();
    gausResi1->Draw("SAME");
    pt->Draw();
    c1->SaveAs((output+"/ResiTestSig_"+name+ch+".pdf").c_str());

    sprintf( text, "Expected bkg. %.0f (%d exp.) ", nBkg, nExp ); 
    hPullBkg->Fit( gausPull2, "WR"); cout<<endl;
    hPullBkg->Draw();
    gausPull2->Draw("SAME");
    pt->Clear();
    pt->AddText(text);
    pt->Draw();
    c1->SaveAs((output+"/PullTestBkg_"+name+ch+".pdf").c_str());

    hResiBkg->Fit( gausResi2, "WR"); cout<<endl;
    hResiBkg->Draw();
    gausResi2->Draw("SAME");
    pt->Draw();
    c1->SaveAs((output+"/ResiTestBkg_"+name+ch+".pdf").c_str());

    cout<<"Input: nExp "<<nExp<<", nSig "<<nSig<<", nBkg "<<nBkg<<endl;
    fout->Write();
}



