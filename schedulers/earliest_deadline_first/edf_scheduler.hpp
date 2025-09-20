class EDF_scheduler : public BaseScheduler {
public:
    void addProcess(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> getNextProcess() override;
    void onProcessComplete(std::shared_ptr<Process> process) override;
    void onTimeSliceExpired(std::shared_ptr<Process> process) override;
    bool hasProcesses() const override;
    std::string getName() const override { return "FCFS"; }
};


