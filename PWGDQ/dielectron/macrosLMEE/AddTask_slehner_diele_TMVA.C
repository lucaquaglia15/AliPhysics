AliAnalysisTask *AddTask_slehner_diele_TMVA(  Int_t trackCut=0,
                                              Int_t PIDCut=0,
                                              Int_t evCut=0,
                                              Double_t centMin=0.,
                                              Double_t centMax=100.,
                                              Bool_t SetPIDCorrection=kFALSE,
                                              Bool_t useAODFilterCuts=kFALSE,
                                              Bool_t hasMC=kFALSE,
                                              TString TMVAweight = "TMVAClassification_BDTG.weights_094.xml",
                                              Int_t maxTrCuts=0,
                                              Int_t maxPIDCuts=0,
                                              Int_t maxMVACut=0       
        
        ){
  
  TString directoryBaseName = "slehnerLMEETMVA";
  TString outputFileName = "AnalysisResults.root";
//  Bool_t isNano=kFALSE;
  //get the current analysis manager
  AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();
  if (!mgr) {
    Error("AddTask_slehner_diele_TMVA", "No analysis manager found.");
    return 0;
  }


  TString configBasePath("$ALICE_PHYSICS/PWGDQ/dielectron/macrosLMEE/");
  TString configFile("Config_slehner_diele_TMVA.C");
  TString configLMEECutLib("LMEECutLib_slehner.C");
  TString configLMEECutLibPath(configBasePath+configLMEECutLib);
  TString configFilePath(configBasePath+configFile);
     
  //load dielectron configuration files
  if (!gROOT->GetListOfGlobalFunctions()->FindObject(configFile.Data()))    gROOT->LoadMacro(configFilePath.Data());
  if (!gROOT->GetListOfGlobalFunctions()->FindObject(configLMEECutLibPath.Data()))    gROOT->LoadMacro(configLMEECutLibPath.Data());
  
  //create task and add it to the manager
  AliAnalysisTaskMultiDielectron *task=new AliAnalysisTaskMultiDielectron("MultiDiEData_slehner_TMVA");

  task->UsePhysicsSelection();

  Int_t triggerNames = AliVEvent::kINT7;//PbPb Min Bias, can be set also from outside
//  if(!isNano){
    task->SelectCollisionCandidates(triggerNames);
    task->SetTriggerMask(triggerNames);
//     task->SetRejectPileup(); // to be done differently (too strong cuts at the moment in dielectron framework) 
//  }

  LMEECutLib* cutlib = new LMEECutLib();
  Bool_t isRun2 = kTRUE;
  
  task->SetEventFilter(cutlib->GetEventCuts(centMin, centMax)); // All cut sets have same event cuts

  mgr->AddTask(task);
  
//      for(Int_t MVACut = 0; MVACut <= 0; ++MVACut){
      Int_t trackCut = 0;
      Int_t PIDCut = 0;
      Int_t MVACut = 0;
      for(trackCut = 0; trackCut <=maxTrCuts; ++trackCut){
        AliDielectron * diel_low = Config_slehner_diele_TMVA(trackCut,PIDCut,MVACut,SetPIDCorrection,hasMC);
        if(!diel_low){
          Printf("=======================================");
          Printf("No AliDielectron object loaded -> EXIT ");
          Printf("=======================================");
          return NULL; 
        }  
        std::cout << "CutTr: "<<trackCut<<" CutPID: "<<PIDCut<<" MVAcut: "<<-1+MVACut*0.2<<" being added"<< std::endl;
        diel_low->GetTrackFilter().AddCuts(SetupTrackCutsAndSettings(trackCut, PIDCut, MVACut, useAODFilterCuts,TMVAweight));   
        task->AddDielectron(diel_low);
        printf("successfully added AliDielectron: %s\n",diel_low->GetName());           
      }
      trackCut = 0;
      for(PIDCut = 0; PIDCut <= maxPIDCuts; ++PIDCut){
        AliDielectron * diel_low = Config_slehner_diele_TMVA(trackCut,PIDCut,MVACut,SetPIDCorrection,hasMC);
        if(!diel_low){
          Printf("=======================================");
          Printf("No AliDielectron object loaded -> EXIT ");
          Printf("=======================================");
          return NULL; 
        }  
        std::cout << "CutTr: "<<trackCut<<" CutPID: "<<PIDCut<<" MVAcut: "<<-1+MVACut*0.2<<" being added"<< std::endl;
        diel_low->GetTrackFilter().AddCuts(SetupTrackCutsAndSettings(trackCut, PIDCut, MVACut, useAODFilterCuts,TMVAweight));   
        task->AddDielectron(diel_low);
        printf("successfully added AliDielectron: %s\n",diel_low->GetName());           
      }      
      PIDCut = 0;
      for(MVACut = 0; MVACut <= maxMVACut; ++MVACut){
        AliDielectron * diel_low = Config_slehner_diele_TMVA(trackCut,PIDCut,MVACut,SetPIDCorrection,hasMC);
        if(!diel_low){
          Printf("=======================================");
          Printf("No AliDielectron object loaded -> EXIT ");
          Printf("=======================================");
          return NULL; 
        }  
        std::cout << "CutTr: "<<trackCut<<" CutPID: "<<PIDCut<<" MVAcut: "<<-1+MVACut*0.2<<" being added"<< std::endl;
        diel_low->GetTrackFilter().AddCuts(SetupTrackCutsAndSettings(trackCut, PIDCut, MVACut, useAODFilterCuts,TMVAweight));   
        task->AddDielectron(diel_low);
        printf("successfully added AliDielectron: %s\n",diel_low->GetName());           
      }      

  //create output container
  AliAnalysisDataContainer *coutput1 =
    mgr->CreateContainer(Form("%s_tree",directoryBaseName.Data()),
                         TTree::Class(),
                         AliAnalysisManager::kExchangeContainer,
                         outputFileName.Data());
  
  AliAnalysisDataContainer *cOutputHist1 =
    mgr->CreateContainer(Form("%sData",directoryBaseName.Data()),
                         TList::Class(),
                         AliAnalysisManager::kOutputContainer,
                         outputFileName.Data());
  
  AliAnalysisDataContainer *cOutputHist2 =
    mgr->CreateContainer(Form("%s_CF",directoryBaseName.Data()),
                         TList::Class(),
                         AliAnalysisManager::kOutputContainer,
                         outputFileName.Data());
  //                         "slehner_diele_PbPb_CF.root");
  
  AliAnalysisDataContainer *cOutputHist3 =
    mgr->CreateContainer(Form("%s_EventStat",directoryBaseName.Data()),
                         TH1D::Class(),
                         AliAnalysisManager::kOutputContainer,
                         outputFileName.Data());
  
  
  mgr->ConnectInput(task,  0, mgr->GetCommonInputContainer());
  mgr->ConnectOutput(task, 0, coutput1 );
  mgr->ConnectOutput(task, 1, cOutputHist1);
  mgr->ConnectOutput(task, 2, cOutputHist2);
  mgr->ConnectOutput(task, 3, cOutputHist3);
  
  return task;
}
