/**
 * @file 
 * 
 * Scripts to draw energy loss fits from correction object file 
 *
 * @ingroup pwglf_forward_scripts_corr
 */
/** 
 * Clear canvas 
 * 
 * @param c Canvas to clear 
 *
 * @ingroup pwglf_forward_scripts_corr
 */
void
ClearCanvas(TCanvas* c)
{
  c->SetLeftMargin(.1);
  c->SetRightMargin(.05);
  c->SetBottomMargin(.1);
  c->SetTopMargin(.05);
  c->Clear();
}

/** 
 * Load the needed libraries
 * 
 */
void
LoadLibraries()
{
  const char* test = gSystem->GetLibraries("PWGLFforward2","D",false);
  if (test && test[0] != '\0') return;
  gROOT->Macro("$ALICE_PHYSICS/PWGLF/FORWARD/analysis2/scripts/LoadLibs.C");
}

/** 
 * Draw energy loss fits to a multi-page PDF. 
 *
 * @par Input: 
 * The input file is expected to contain a AliFMDCorrELossFit object
 * named @c elossfits in the top level directory.
 * 
 * @par Output: 
 * A multi-page PDF.  Note, that the PDF generated by ROOT in this way
 * is broken (cannot be read by Acrobat Reader on Windows and MacOSX)
 * and one should pass it through a filter to correct these problems.
 * 
 * @param fname     File name 
 * @param option    Drawing options 
 * @param tracklets Tracklets flag
 *
 * @ingroup pwglf_forward_scripts_corr
 */
void
DrawCorrCentralSecMap2(const char* fname, const char* option="colz", bool tracklets=true)
{
  //__________________________________________________________________
  // Load libraries and object 
  LoadLibraries();

  TFile* file = TFile::Open(fname, "READ");
  if (!file) { 
    Error("DrawCorrSecMap", "Failed to open %s", fname);
    return;
  }
  TString pname(fname);
  pname.ReplaceAll(".root", ".png");
  pname = gSystem->BaseName(pname);

  AliCentralMultiplicityTask::Manager* mgr = 
    new AliCentralMultiplicityTask::Manager;
  const char* objName = mgr->GetObjectName(0);
  AliCentralCorrSecondaryMap* corr = 
    static_cast<AliCentralCorrSecondaryMap*>(file->Get(objName));
  if (!corr) { 
    Error("DrawCorrCentralSecMap", "Object '%s' not found in %s", 
	  objName, fname);
    file->ls();
    return;
  }

  //__________________________________________________________________
  // Create a canvas
  Int_t w = 1200;
  TCanvas* c = new TCanvas("c", "c", w, w / TMath::Sqrt(2));
  c->SetFillColor(0);
  c->SetBorderSize(0);
  c->SetBorderMode(0);
  // c->Print(Form("%s[", pname.Data()));
  
  gStyle->SetOptStat(0);
  gStyle->SetTitleColor(0);
  gStyle->SetTitleStyle(0);
  gStyle->SetTitleBorderSize(0);
  gStyle->SetTitleX(.1);
  gStyle->SetTitleY(1);
  gStyle->SetTitleW(.8);
  gStyle->SetTitleH(.09);
  gStyle->SetFrameFillColor(kWhite);
  gStyle->SetFrameBorderSize(1);
  gStyle->SetFrameBorderMode(1);
  gStyle->SetPalette(1);

  TString opt(option);
  opt.ToLower();
  Bool_t h2d = (opt.Contains("lego") || 
		opt.Contains("surf") || 
		opt.Contains("col"));
  ClearCanvas(c);
  //__________________________________________________________________
  // Draw all corrections
  const TAxis& vtxAxis = corr->GetVertexAxis();
  Int_t        nVtx    = vtxAxis.GetNbins();
  c->Divide((nVtx+2)/3, 3, 0, 0);
  Int_t ipad = 0;
  for (UShort_t v=1; v <= nVtx+1; v++) { 
    ipad++;
    if (ipad == 1) {
      c->cd(ipad);
      TLatex* l = new TLatex(.5, .5, 
			     "#frac{#sum N_{ch,SPD0}}{#sum N_{ch,primary}}");
      if (!tracklets) 
	l->SetText(.5,.5,"#frac{dN_{ch}/d#eta}{#sum N_{ch,primary}}");
      l->SetNDC();
      l->SetTextAlign(22);
      l->SetTextSize(.1);
      l->Draw();
      ipad++;
    }
    if (ipad == 12) {
      if (!tracklets) 
	continue;
      c->cd(ipad);
      TFile* f = TFile::Open("forward_mccorr.root", "READ");
      if (!f) {
	Warning("DrawCorrCentralSecMap2", "File forward_mccorr.root not found");
	continue;
      }
      TList* l3 = static_cast<TList*>(f->Get("CentralSums"));
      if (!l3) { 
	Warning("DrawCorrCentralSecMap2", "No CentralSums list found");
	f->Close();
	continue;
      }
      TH1* xyz = static_cast<TH1*>(l3->FindObject("xyz"));
      if (!xyz) {
	Warning("DrawCorrCentralSecMap2", "no xyz histogram found");
	f->Close();
	continue;
      }
      xyz = static_cast<TH1*>(xyz->Clone());
      xyz->SetDirectory(0);
      xyz->Draw("ISO");
      f->Close();
      continue;
    }
    TVirtualPad* p = c->cd(ipad);
    p->SetFillColor(kWhite);
    p->SetGridx();
    p->SetGridy();

    TH2*   h1 = corr->GetCorrection(v);
    if (h2d) { 
      p->SetRightMargin(0.13);
      h1->SetMaximum(1.9);
      h1->Draw(option);
      continue;
    }

    TH1D*  pr = h1->ProjectionX(Form("vtxbin%02d", v), -1, -1, "e");
    TH1D*  nr = static_cast<TH1D*>(pr->Clone("norm"));
    nr->SetDirectory(0);
    pr->SetDirectory(0);
    pr->SetTitle(Form("%+5.1f<v_{z}<%+5.1f", 
		      vtxAxis.GetBinLowEdge(v),
		      vtxAxis.GetBinUpEdge(v)));
    pr->SetMarkerColor(kRed+1);
    pr->SetFillColor(kRed+1);
    pr->SetFillStyle(3001);
    pr->SetMaximum(1.65);
    pr->GetXaxis()->SetRangeUser(-3.1,3.1);
    
    Int_t nX = h1->GetNbinsX();
    Int_t nY = h1->GetNbinsY();
    nr->Reset();
    for (Int_t i = 1; i <= nX; i++) { 
      Int_t nonZero = 0;
      for (Int_t j = 1; j <= nY; j++) 
	if (h1->GetBinContent(i,j) > 0.001) nonZero++;
      nr->SetBinContent(i, nonZero);
    }
    // pr->Scale(1. / nY);
    pr->Divide(nr);
    pr->Draw("hist"); 
    pr->Draw("same");
   
  }
  
  //__________________________________________________________________
  // Close output file 
  c->SaveAs(pname.Data());
  
}
//
// EOF
//
