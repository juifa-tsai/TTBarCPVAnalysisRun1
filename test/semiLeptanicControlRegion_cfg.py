import FWCore.ParameterSet.Config as cms

from FWCore.ParameterSet.VarParsing import VarParsing
from inputFiles_cfi import * 
from inputJsons_cfi import * 

options = VarParsing('python')

options.register('outFilename', 'SemiLeptanicAnalysis.root',
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Output file name"
    )
options.register('MaxEvents', -1,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.int,
    "Run events max"
    )
options.register('reportEvery', 1000,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.int,
    "Report every N events (default is N=1000)"
    )
options.register('ttreedir', 'bprimeKit',
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Name of ROOT TTree dir: Either 'ntuple' or 'skim' or 'bVeto'"
    )
options.register('HistPUDistMC', 'pileup_mc',
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Name of Histogram for MC pileup weights"
    )
options.register('HistPUDistData', 'pileup_data',
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Name of Histogram for data pileup weights"
    )
options.register('dRIsoLeptonFromJets', 0.5,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.float,
    "isolate lepton with deltaR( lepton, jet )"
    )
options.register('dRrmElelectronOverlapeMuon', 0.3,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.float,
    "Remove electron overlapping with muon deltaR( el, mu )"
    )
options.register('MaxChi2', 40,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.float,
    "Event selection for hadronic top max of min_chi2"
    )
options.register('MinChi2',  0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.float,
    "Event selection for hadronic top min of min_chi2"
    )
options.register('NJets', 4,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Number of jets"
    )
options.register('NbJetsCSVM', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Number of CSVM b-jets"
    )
options.register('NbJetsCSVL', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Number of CSVL b-jets"
    )
options.register('ShiftJER', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Systemtic unc. shift of JER sf: 0(nominal), 1(sigma), -1(-sigma)"
    )
options.register('ShiftBTagSF', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Systemtic unc. shift of btag sf: 0(nominal), 1(sigma), -1(-sigma)"
    )
options.register('ShiftTopPtReWeight', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Systemtic unc. shift of Top Pt Re-Weight: 0(nominal), 1(sigma), -1(-sigma)"
    )
options.register('ShiftTightMuonIDSF', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Systemtic unc. shift of tight muon id sf: 0(nominal), 1(sigma), -1(-sigma)"
    )
options.register('ShiftTightMuonIsoSF', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Systemtic unc. shift of tight muon iso sf: 0(nominal), 1(sigma), -1(-sigma)"
    )
options.register('ShiftTightElectronIDSF', 0,
    VarParsing.multiplicity.singleton,    
    VarParsing.varType.int,
    "Systemtic unc. shift of tight electron id sf: 0(nominal), 1(sigma), -1(-sigma)"
    )
options.register('Owrt', 'MT:3',
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Weight the obseverble in top mass"
    )
options.register('DoSaveTree', False,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Store tree for selected events"
    )
options.register('Debug', False,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Print out Debug info during run"
    )
options.parseArguments()

process = cms.Process("SemiLeptanic")
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1) ) 

process.source = cms.Source("EmptySource")
process.TFileService = cms.Service("TFileService",
    fileName = cms.string(options.outFilename) 
)

MT = 173.0
Oweight = 1.0
power = int(options.Owrt.split(':')[1])
i = 0
while ( i < power ):
    Oweight *= MT
    i+=1

isSkim = False
if options.ttreedir.lower() == 'skim':
    isSkim = True

from TTBarCPV.TTBarCPVAnalysisRun1.Selector_Vertex_cfi   import*
from TTBarCPV.TTBarCPVAnalysisRun1.Selector_Jet_cfi      import*
from TTBarCPV.TTBarCPVAnalysisRun1.Selector_BJet_cfi     import*
from TTBarCPV.TTBarCPVAnalysisRun1.Selector_nonBJet_cfi  import*
from TTBarCPV.TTBarCPVAnalysisRun1.Selector_Lepton_cfi   import*
from TTBarCPV.TTBarCPVAnalysisRun1.Selector_Muon_cfi     import*
from TTBarCPV.TTBarCPVAnalysisRun1.Selector_Electron_cfi import*

process.SemiLeptanic = cms.EDAnalyzer('SemiLeptanicControlRegion',
    MaxEvents                   = cms.int32(options.MaxEvents),
    ReportEvery                 = cms.int32(options.reportEvery),  
    InputTTree                  = cms.string(options.ttreedir+'/root'),
    InputJsons                  = cms.vstring(JsonNames), 
    InputFiles                  = cms.vstring(FileNames), 
    #InputFiles                  = cms.vstring(FileNames_BprimtKits_NTUG3_SemiLeptTest), 
    #InputFiles                  = cms.vstring(FileNames_BprimtKits_SemiLeptTestSkim),
    #InputFiles                  = cms.vstring(FileNames_BprimtKits_SemiLeptTestSkimData),
    #InputFiles                  = cms.vstring(FileNames_BprimtKits_SemiLept),
    HLT_MuChannel               = cms.vint32( 2868,3244,3542,4204,4205,4827,5106,5573  ), # HLT_IsoMu24_eta2p1_v*
    HLT_ElChannel               = cms.vint32( 3155,3496,4002,4003,4004,5043 ),            # HLT_Ele27_WP80_v* 
    File_PUDistMC               = cms.string('pileup_Data_Summer12_53X_S10.root'),
    File_PUDistData             = cms.string('pileup_Data_Summer12_53X_S10.root'),
    Hist_PUDistMC               = cms.string(options.HistPUDistMC),
    Hist_PUDistData             = cms.string(options.HistPUDistData),
    SelPars_Vertex              = defaultVertexSelectionParameters.clone(), 
    SelPars_Jet                 = defaultJetSelectionParameters.clone(), 
    SelPars_CSVM_BJet           = defaultBJetSelectionParameters.clone(
                                jetType = cms.string('CSVM_BJet') 
                                ), 
    SelPars_CSVL_BJet           = defaultBJetSelectionParameters.clone(
                                jetType = cms.string('CSVL_BJet'), 
                                jetCombinedSVBJetTagsMin = cms.double(0.244)
                                ), 
    SelPars_NonBJet             = defaultNonBJetSelectionParameters.clone( 
                                jetCombinedSVBJetTagsMax = cms.double(0.244)
                                ),
    SelPars_LooseLepton         = defaultLeptonSelectionParameters.clone(), 
    SelPars_TightMuon           = defaultMounSelectionParameters.clone(), 
    SelPars_TightElectron       = defaultElectronSelectionParameters.clone(),
    dR_IsoLeptonFromJets        = cms.double(options.dRIsoLeptonFromJets),
    dR_rmElelectronOverlapeMuon = cms.double(options.dRrmElelectronOverlapeMuon),
    MaxChi2                     = cms.double(options.MaxChi2),
    MinChi2                     = cms.double(options.MinChi2),
    Owrt                        = cms.double(Oweight), 
    NJets                       = cms.int32(options.NJets),
    NbJetsCSVM                  = cms.int32(options.NbJetsCSVM),
    NbJetsCSVL                  = cms.int32(options.NbJetsCSVL),
    Shift_JER                   = cms.int32(options.ShiftJER),
    Shift_BTagSF                = cms.int32(options.ShiftBTagSF),
    Shift_TopPtReWeight         = cms.int32(options.ShiftTopPtReWeight),
    Shift_TightMuonIDSF         = cms.int32(options.ShiftTightMuonIDSF),
    Shift_TightMuonIsoSF        = cms.int32(options.ShiftTightMuonIsoSF),
    Shift_TightElectronIDSF     = cms.int32(options.ShiftTightElectronIDSF),
    Debug                       = cms.bool(options.Debug),
    IsSkim                      = cms.bool(isSkim),
    DoSaveTree                  = cms.bool(options.DoSaveTree), 
) 

process.SimpleMemoryCheck = cms.Service("SimpleMemoryCheck",ignoreTotal = cms.untracked.int32(1) )
process.p = cms.Path(process.SemiLeptanic)

