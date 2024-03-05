#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-C"<< std::endl;
    pid_t runningJobPid = smash.current_jobpid;
    std::string runningCommand=smash.current_jobcmdline;
    if(runningJobPid == 0)
    {
        return;
    }
    JobsList::JobEntry* job=smash.jobs_list->getJobByPId(runningJobPid);
    if(job != nullptr)
    {
        smash.jobs_list->removeJobById(job->id);
    }
    if(kill(runningJobPid, SIGKILL) == -1) {
        Failed_syscall("kill");
        return;
    }
    std::cout << "smash: process " << runningJobPid << " was killed" << std::endl;

    smash.current_jobpid=0;
    smash.current_jobcmdline="";
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}
