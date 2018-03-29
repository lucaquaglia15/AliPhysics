/************************************************************************************
 * Copyright (C) 2017, Copyright Holders of the ALICE Collaboration                 *
 * All rights reserved.                                                             *
 *                                                                                  *
 * Redistribution and use in source and binary forms, with or without               *
 * modification, are permitted provided that the following conditions are met:      *
 *     * Redistributions of source code must retain the above copyright             *
 *       notice, this list of conditions and the following disclaimer.              *
 *     * Redistributions in binary form must reproduce the above copyright          *
 *       notice, this list of conditions and the following disclaimer in the        *
 *       documentation and/or other materials provided with the distribution.       *
 *     * Neither the name of the <organization> nor the                             *
 *       names of its contributors may be used to endorse or promote products       *
 *       derived from this software without specific prior written permission.      *
 *                                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
 * DISCLAIMED. IN NO EVENT SHALL ALICE COLLABORATION BE LIABLE FOR ANY              *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES       *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND      *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS    *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     *
 ************************************************************************************/
#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <vector>

#include <THistManager.h>
#include <TLinearBinning.h>

#include "AliAnalysisManager.h"
#include "AliAnalysisTaskEmcalJetEnergySpectrum.h"
#include "AliEmcalAnalysisFactory.h"
#include "AliEmcalJet.h"
#include "AliEmcalTriggerDecisionContainer.h"
#include "AliEmcalTriggerStringDecoder.h"
#include "AliInputEventHandler.h"
#include "AliJetContainer.h"
#include "AliLog.h"
#include "AliVEvent.h"

ClassImp(EmcalTriggerJets::AliAnalysisTaskEmcalJetEnergySpectrum);

using namespace EmcalTriggerJets;

AliAnalysisTaskEmcalJetEnergySpectrum::AliAnalysisTaskEmcalJetEnergySpectrum(const char *name):
  AliAnalysisTaskEmcalJet(name, true),
  fHistos(nullptr),
  fIsMC(false),
	fTriggerSelectionBits(AliVEvent::kAny),
  fTriggerSelectionString(""),
  fNameTriggerDecisionContainer("EmcalTriggerDecision"),
  fUseTriggerSelectionForData(true),
  fUseDownscaleWeight(false),
  fNameJetContainer("datajets")
{
  SetUseAliAnaUtils(true);
}

AliAnalysisTaskEmcalJetEnergySpectrum::~AliAnalysisTaskEmcalJetEnergySpectrum(){
  if(fHistos) delete fHistos;
}

void AliAnalysisTaskEmcalJetEnergySpectrum::UserCreateOutputObjects(){
  AliAnalysisTaskEmcalJet::UserCreateOutputObjects();

  TLinearBinning jetptbinning(200, 0., 200.), etabinning(100, -1., 1.), phibinning(100., 0., 7.), nefbinning(100, 0., 1.), trgclusterbinning(5, -1.5, 3.5);
  const TBinning *binnings[5] = {&jetptbinning, &etabinning, &phibinning, &nefbinning, &trgclusterbinning};
  fHistos = new THistManager(Form("Histos_%s", GetName()));
  fHistos->CreateTH1("hEventCounter", "Event counter histogram", 1, 0.5, 1.5);
  fHistos->CreateTH1("hClusterCounter", "Event counter histogram", 5, -1.5, 3.5);
  fHistos->CreateTHnSparse("hJetTHnSparse", "jet thnsparse", 5, binnings, "s");

  for(auto h : *fHistos->GetListOfHistograms()) fOutput->Add(h);
  PostData(1, fOutput);
}

bool AliAnalysisTaskEmcalJetEnergySpectrum::Run(){
  auto datajets = this->GetJetContainer(fNameJetContainer);
  if(!datajets) {
    AliErrorStream() << "Jet container " << fNameJetContainer << " not found" << std::endl;
    return false;
  }
  if(!TriggerSelection()) return false;

  auto trgcluster = GetTriggerClusterIndex(fInputEvent->GetFiredTriggerClasses());
  fHistos->FillTH1("hEventCounter", 1);
  fHistos->FillTH1("hClusterCounter", trgcluster);
  for(auto j : datajets->accepted()){
    double datapoint[5] = {j->Pt(), j->Eta(), j->Phi(), j->NEF(), static_cast<double>(trgcluster)};
    fHistos->FillTHnSparse("hJetTHnSparse", datapoint);
  }
  return true;
}

Int_t AliAnalysisTaskEmcalJetEnergySpectrum::GetTriggerClusterIndex(const TString &triggerstring) const {
  // decode trigger string in order to determine the trigger clusters
  int clusterindex = -1;
  std::vector<std::string> clusternames;
  auto triggerinfos = PWG::EMCAL::DecodeTriggerString(triggerstring.Data());
  for(auto t : triggerinfos) {
    if(std::find(clusternames.begin(), clusternames.end(), t.fTriggerCluster) == clusternames.end()) clusternames.emplace_back(t.fTriggerCluster);
  }
  bool isCENT = (std::find(clusternames.begin(), clusternames.end(), "CENT") != clusternames.end()),
       isCENTNOTRD = (std::find(clusternames.begin(), clusternames.end(), "CENTNOTRD") != clusternames.end()),
       isCALO = (std::find(clusternames.begin(), clusternames.end(), "CALO") != clusternames.end()),
       isCALOFAST = (std::find(clusternames.begin(), clusternames.end(), "CALOFAST") != clusternames.end());
  if(isCENT) clusterindex = 0;
  else if(isCENTNOTRD) clusterindex = 1;
  else if(isCALO) clusterindex = 2;
  else if(isCALOFAST) clusterindex = 3;
  return clusterindex;
}

bool AliAnalysisTaskEmcalJetEnergySpectrum::TriggerSelection() const {
  if(!fIsMC){
    // Pure data - do EMCAL trigger selection from selection string
    if(!(fInputHandler->IsEventSelected() & fTriggerSelectionBits)) return false;
    if(fTriggerSelectionString.Length()) {
      if(!fInputEvent->GetFiredTriggerClasses().Contains(fTriggerSelectionString)) return false;
      if(fTriggerSelectionString.Contains("EJ") && fUseTriggerSelectionForData) {
        auto trgselresult = static_cast<PWG::EMCAL::AliEmcalTriggerDecisionContainer *>(fInputEvent->FindListObject(fNameTriggerDecisionContainer));
        AliDebugStream(1) << "Found trigger decision object: " << (trgselresult ? "yes" : "no") << std::endl;
        if(!trgselresult){
          AliErrorStream() <<  "Trigger decision container with name " << fNameTriggerDecisionContainer << " not found in event - not possible to select EMCAL triggers" << std::endl;
          return false;
        }
        if(!trgselresult->IsEventSelected(fTriggerSelectionString)) return false;
      }
    }
  } else {
    if(IsSelectEmcalTriggers(fTriggerSelectionString.Data())){
      // Simulation - do EMCAL trigger selection from trigger selection object
      auto mctrigger = static_cast<PWG::EMCAL::AliEmcalTriggerDecisionContainer *>(fInputEvent->FindListObject(fNameTriggerDecisionContainer));
      AliDebugStream(1) << "Found trigger decision object: " << (mctrigger ? "yes" : "no") << std::endl;
      if(!mctrigger){
        AliErrorStream() <<  "Trigger decision container with name " << fNameTriggerDecisionContainer << " not found in event - not possible to select EMCAL triggers" << std::endl;
        return false;
      }
      if(!mctrigger->IsEventSelected(fTriggerSelectionString)) return false;
    }
  }
  return true;
}

bool AliAnalysisTaskEmcalJetEnergySpectrum::IsSelectEmcalTriggers(const std::string &triggerstring) const {
  const std::array<std::string, 8> kEMCALTriggers = {
    "EJ1", "EJ2", "DJ1", "DJ2", "EG1", "EG2", "DG1", "DG2"
  };
  bool isEMCAL = false;
  for(auto emcaltrg : kEMCALTriggers) {
    if(triggerstring.find(emcaltrg) != std::string::npos) {
      isEMCAL = true;
      break;
    }
  }
  return isEMCAL;
}


AliAnalysisTaskEmcalJetEnergySpectrum *AliAnalysisTaskEmcalJetEnergySpectrum::AddTaskJetEnergySpectrum(Bool_t isMC, AliJetContainer::EJetType_t jettype, double radius, const char *trigger, const char *suffix){
  AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();
  if(!mgr) {
    std::cerr << "Analysis manager not initialized" << std::endl;
    return nullptr;
  }

  std::string jettypestring;
  UInt_t acctype(AliJetContainer::kTPCfid);
  switch(jettype){
    case AliJetContainer::kChargedJet:  jettypestring = "ChargedJets"; acctype = AliJetContainer::kTPCfid; break;
    case AliJetContainer::kFullJet:     jettypestring = "FullJets";    acctype = AliJetContainer::kEMCALfid; break;
    case AliJetContainer::kNeutralJet:  jettypestring = "NeutralJets"; acctype = AliJetContainer::kTPCfid; break;
  };

  std::stringstream tag, outfilename;
  tag << jettypestring << "_R" << std::setw(2) << std::setfill('0') << radius << "_" << trigger;
  if(strlen(suffix)) {
    tag << "_" << suffix;
  }
  auto task = new AliAnalysisTaskEmcalJetEnergySpectrum(Form("JetEnergySpectrum_%s", tag.str().data()));
  task->SetIsMC(isMC);
  mgr->AddTask(task);

  // Connect particle and cluster container
  AliTrackContainer *tracks(nullptr);
  AliClusterContainer *clusters(nullptr);
  if(jettype == AliJetContainer::kChargedJet || jettype == AliJetContainer::kFullJet) {
    tracks = task->AddTrackContainer("usedefault");
    tracks->SetMinPt(0.15);
  }
  if(jettype == AliJetContainer::kNeutralJet || jettype == AliJetContainer::kChargedJet){
    clusters = task->AddClusterContainer("usedefault");
    clusters->SetDefaultClusterEnergy(AliVCluster::kHadCorr);
    clusters->SetClusHadCorrEnergyCut(0.3);
  }


  // Create proper jet container
  auto jetcont = task->AddJetContainer(jettype, AliJetContainer::antikt_algorithm, AliJetContainer::E_scheme, radius, acctype, tracks, clusters);
  jetcont->SetName("datajets");
  task->SetNameJetContainer("datajets");

  // Link input and output container
  outfilename << mgr->GetCommonFileName() << ":JetSpectrum_" << tag.str().data();
  mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer());
  mgr->ConnectOutput(task, 1, mgr->CreateContainer(Form("JetSpectrum_%s", tag.str().data()), TList::Class(), AliAnalysisManager::kOutputContainer, outfilename.str().data()));

  return task;
}