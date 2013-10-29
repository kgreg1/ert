from ert_gui.ide.keywords.advanced_keywords import AdvancedKeywords
from ert_gui.ide.keywords.analysis_module_keywords import AnalysisModuleKeywords
from ert_gui.ide.keywords.definitions import ConfigurationLineDefinition
from ert_gui.ide.keywords.eclipse_keywords import EclipseKeywords
from ert_gui.ide.keywords.enkf_control_keywords import EnkfControlKeywords
from ert_gui.ide.keywords.ensemble_keywords import EnsembleKeywords
from ert_gui.ide.keywords.parametrization_keywords import ParametrizationKeywords
from ert_gui.ide.keywords.plot_keywords import PlotKeywords
from ert_gui.ide.keywords.qc_keywords import QCKeywords
from ert_gui.ide.keywords.queue_system_keywords import QueueSystemKeywords
from ert_gui.ide.keywords.report_keywords import ReportKeywords
from ert_gui.ide.keywords.run_keywords import RunKeywords
from ert_gui.ide.keywords.simulation_control_keywords import SimulationControlKeywords
from ert_gui.ide.keywords.unix_environment_keywords import UnixEnvironmentKeywords
from ert_gui.ide.keywords.workflow_keywords import WorkflowKeywords


class ErtKeywords(object):
    def __init__(self):
        super(ErtKeywords, self).__init__()

        self.keywords = {}

        EnsembleKeywords(self)
        RunKeywords(self)
        EclipseKeywords(self)
        QueueSystemKeywords(self)
        SimulationControlKeywords(self)
        ParametrizationKeywords(self)
        EnkfControlKeywords(self)
        AnalysisModuleKeywords(self)
        PlotKeywords(self)
        WorkflowKeywords(self)
        ReportKeywords(self)
        AdvancedKeywords(self)
        QCKeywords(self)
        UnixEnvironmentKeywords(self)

    def addKeyword(self, keyword):
        assert isinstance(keyword, ConfigurationLineDefinition)

        name = keyword.keywordDefinition().name()
        if name in self.keywords:
            raise ValueError("Keyword %s already in Ert keyword list!" % name)

        self.keywords[name] = keyword

    def __contains__(self, item):
        return item in self.keywords

    def __getitem__(self, item):
        """ @rtype: ConfigurationLineDefinition """
        return self.keywords[item]

