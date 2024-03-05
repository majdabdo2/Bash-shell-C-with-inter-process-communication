#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>

#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

void Command::removeBackgroundSign()
{
    char* hi=strdup(cmd_line);
    if(hi == nullptr)
    {
        return;
    }

    _removeBackgroundSign(hi);
    _parseCommandLine(hi,argv);
    free(hi);
}

SmallShell::SmallShell() : prompt("smash") {
    this->pid=getpid();
    this->cnt=0;
    this->last_dir="";
    this->jobs_list = new JobsList();
}

SmallShell::~SmallShell()
{
    delete this->jobs_list;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (firstWord[firstWord.size() - 1] == '&') {
        firstWord.pop_back();
    }

  if(cmd_s.find(">")!=string::npos) {
      return new RedirectionCommand(cmd_line);
  }

  if(cmd_s.find("|")!=string::npos){
      return new PipeCommand(cmd_line);
  }

  if (firstWord.compare("chprompt") == 0) {
      return new chprompt(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line,NULL);
  }
  else if (firstWord.compare("jobs") == 0) {
      return new JobsCommand(cmd_line,SmallShell::getInstance().jobs_list);
  }
  else if (firstWord.compare("fg") == 0) {
      return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0) {
      return new QuitCommand(cmd_line,SmallShell::getInstance().jobs_list);
  }
  else if (firstWord.compare("kill") == 0) {
      return new KillCommand(cmd_line,SmallShell::getInstance().jobs_list);
  }
  else if (firstWord.compare("chmod") == 0) {
      return new ChmodCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    jobs_list->removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
  if(cmd != nullptr)
  {
      std::string quit= string(cmd->argv[0]);
      cmd->execute();
      delete(cmd);
      if(quit == "quit")
      {
          exit(0);
      }
  }
}

void chprompt::execute(){
    if(argc > 1)
    {
        SmallShell::getInstance().prompt = string(argv[1]);
    }
    else
    {
        SmallShell::getInstance().prompt = "smash";
    }
}

void ShowPidCommand::execute() {
   // removeBackgroundSign();
    std::cout << "smash pid is " << SmallShell::getInstance().pid << std::endl;
    return;
}

void Failed_syscall(const char* syscall) {
    string str= "smash error: " + string(syscall) + " failed";
    const char *error = (str).c_str();
    perror(error);
}

void GetCurrDirCommand::execute() {
    char* dir = getcwd(NULL,0);
    if(dir)
    {
        std::cout << dir << std::endl;
    }
    else
        Failed_syscall("getcwd");
}

void ChangeDirCommand::execute() {
    if(argc>2)
    {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        return;
    }
    SmallShell& smash= SmallShell::getInstance();
    int cdCount = smash.cnt++;
    std::string newPath;
    char* dir = getcwd(NULL,0);
    if(!dir)
    {
        Failed_syscall("getcwd");
        return;
    }
    if (std::string(argv[1]) == "-") {
        if (cdCount == 0) {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            return;
        }
        newPath = smash.last_dir;
        }
        else{
            newPath = argv[1];
        }

    if (chdir(newPath.c_str()) == -1) {
        Failed_syscall("chdir");
        return;
    }
    smash.last_dir = dir;

}

void JobsCommand::execute() {
    SmallShell& smash= SmallShell::getInstance();
    smash.jobs_list->printJobsList();
}

void JobsList::addJob(std::string cmd_line, pid_t pid){
    removeFinishedJobs();
    int id=1;
    if(!jobs_vec.empty())
    {
        id=jobs_vec[jobs_vec.size()-1]->id+1;
    }
    JobEntry*  new_entry=new JobEntry(id,time(nullptr),pid,cmd_line);
    jobs_vec.insert(jobs_vec.end(),new_entry);
}

void JobsList::printJobsList() {
    SmallShell& smash= SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();

    for(auto & i : jobs_vec) {
        std::cout << "[" << i->id << "] " << i->cmd << std::endl;
    }
}


void JobsList::killAllJobs(){
    for (const auto& job : jobs_vec)
    {
        if(kill(job->pid,SIGKILL) != 0){
            Failed_syscall("Kill");
        }
    }
}

void JobsList::removeFinishedJobs(){
    for (unsigned int i=0 ; i<jobs_vec.size() ;i++)
    {
        if(waitpid(jobs_vec[i]->pid, nullptr,WNOHANG))
        {
            jobs_vec.erase(jobs_vec.begin()+i);
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
    for(auto & job : jobs_vec){
        if(job->id == jobId){
            return job;
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getJobByPId(pid_t pid)
{
    for(auto & job : jobs_vec){
        if(job->pid == pid){
            return job;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId)
{
    for (unsigned int i = 0; i < jobs_vec.size(); i++) {
        if (jobs_vec[i]->id == jobId) {
            jobs_vec.erase(jobs_vec.begin() + i);
        }
    }
}


JobsList::JobEntry *JobsList::maxJobId() {
    if(jobs_vec.empty())
    {
        return nullptr;
    }
    return jobs_vec[jobs_vec.size()-1];
}

bool isValid(std::string str) {
    unsigned int i = 0;
    if (str[0] == '-' && str.size() == 1)
        return false;
    if (str[0] == '-')
        i++;
    for ( ;i < str.size(); i++)
        if (str[i] < '0' || str[i] > '9') {
            return false;
        }
    return true;
}

void handleError(const std::string& message, const std::string& arg = "") {
    std::cerr << "smash error: " ;
    if (arg.compare("jobs list is empty")!=0) {
        if(message.compare("fg: invalid arguments") != 0)
        {
            std::cerr << message << " "<< arg<<" does not exist";

        }
        else std::cerr << message;
    }
    else
    {
        std::cerr << "fg: jobs list is empty";
    }
    std::cerr<<std::endl;
}

JobsList::JobEntry* getJobEntry(int argc, char* argv[]) {
    SmallShell& smash = SmallShell::getInstance();
    if (argc == 2) {
        return smash.jobs_list->getJobById(std::stoi(argv[1]));
    } else {
        return smash.jobs_list->maxJobId();
        }
}

void ForegroundCommand::execute() {
    if ((argc == 2 && !isValid(std::string(argv[1])))) {
        handleError("fg: invalid arguments");
        return;
    }

    JobsList::JobEntry* jobEntry = getJobEntry(argc, argv);

    if (jobEntry == nullptr) {
        handleError("fg: job-id", argc >= 2 ? argv[1] : "jobs list is empty");
        return;
    }

    if(argc > 2)
    {
        handleError("fg: invalid arguments");
        return;
    }

    std::cout << jobEntry->cmd << " " << jobEntry->pid << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.current_jobpid = jobEntry->pid;
    smash.current_jobcmdline = jobEntry->cmd;

    if (kill(jobEntry->pid, SIGCONT) == -1) {
        Failed_syscall("kill");
    }
    if (waitpid(jobEntry->pid, nullptr, WUNTRACED) == -1) {
        Failed_syscall("waitpid");
    }

    smash.jobs_list->removeJobById(jobEntry->id);

    smash.jobs_list->removeFinishedJobs();//not sure?
    smash.current_jobpid = 0;
    smash.current_jobcmdline="";
}




void QuitCommand::execute() {
    SmallShell& smash= SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    if(argc > 1 && string(argv[1])=="kill")
    {
        std::cout << "smash: sending SIGKILL signal to " <<jobs->jobs_vec.size()<<" jobs:" << std::endl;
        for (auto job : jobs->jobs_vec) {
            std::cout << job->pid << ": " << job->cmd << std::endl;
        }
        jobs->killAllJobs();
    }
}

bool isnumber(std::string str) {
    for (unsigned int i = 0; i < str.size(); i++) {
        if (!isdigit(str[i])) {
            if (!(i == 0 && str.size() > 1 && str[i] == '-')) {
                return false;
            }
        }
    }
    return true;
}

void KillCommand::execute() {
    SmallShell& smash= SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    if (argc<=2 || !isnumber(argv[2])) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }
    std::string signalStr = std::string(argv[1]).substr(1);
    int jobId = std::stoi(argv[2]);
    JobsList::JobEntry* jobEntry = nullptr;
    for (const auto& job : jobs->jobs_vec) {
        if (job->id == jobId) {
            jobEntry = job;
            break;
        }
    }
    if (!jobEntry) {
        std::cerr << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
        return;
    }
    if(string(argv[1]).at(0) != '-' || !isnumber(argv[1]) || argc != 3)
    {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }
    if (kill(jobEntry->pid, std::stoi(signalStr)) == -1) {
        Failed_syscall("kill");
        return;
    }
    std::cout << "signal number " << signalStr << " was sent to pid " << jobEntry->pid << std::endl;

    int signalNum = std::stoi(signalStr);

    if (signalNum == SIGKILL) {
        jobs->removeJobById(jobId);
    }
}


void ExternalCommand::execute() {
    if(argc == 0 || cmd_line==0)
    {
        return;
    }

    char copied_cmd_line[strlen(cmd_line) + 1];
    strcpy(copied_cmd_line, cmd_line);


    bool is_background = false;
    if (_isBackgroundComamnd(cmd_line)) {
        _removeBackgroundSign(copied_cmd_line);
        is_background = true;
    }

    char *parsed_cmd[COMMAND_MAX_ARGS];
    _parseCommandLine(copied_cmd_line, parsed_cmd);

    const char *complex_cmd_params[] = {"/bin/bash", "-c", copied_cmd_line, nullptr};

    string cmd_line_string = string(cmd_line);

    bool is_complex =
            ((cmd_line_string.find("*") != std::string::npos) || (cmd_line_string.find("?") != std::string::npos));


    pid_t pid = fork();
    int error;
    if (pid == 0) {
        setpgrp();
        if (is_complex) {
            error = execvp("/bin/bash", (char **) complex_cmd_params);
        } else {
            error = execvp(argv[0], parsed_cmd);
        }
        if (error == -1) {
            Failed_syscall("execvp");
            is_background = false;
            exit(0);
        }
    } else if (pid > 0) {  // Parent process
        SmallShell& smash = SmallShell::getInstance();
        if (is_background) {
            smash.jobs_list->addJob(this->cmd_line, pid);
        } else {
            smash.current_jobpid = pid;
            smash.current_jobcmdline = cmd_line;
            if (waitpid(pid, nullptr, WUNTRACED) == -1) {
                Failed_syscall("waitpid");
                return;
            }
            smash.current_jobpid = 0;
            smash.current_jobcmdline = "";
        }
    } else {
        Failed_syscall("fork");
    }
}


void RedirectionCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();

    string str(cmd_line);
    int flags;
    if(str.find(">>")!=string::npos)
    {
        flags = O_RDWR | O_CREAT | O_APPEND;
    }
    else
        flags = O_WRONLY | O_CREAT | O_TRUNC;

    pid_t pid = fork();
    if (pid == 0) {
        setpgrp();
        int new_fd = open(command[1].c_str(),flags,0655);
        if(new_fd == -1)
        {
            Failed_syscall("open");
            return;
        }
        if (close(1) == -1) {
            Failed_syscall("close");
            exit(0);
        }
        if (dup2(new_fd, 1) == -1) {
            Failed_syscall("dup2");
            return;
        }
        smash.executeCommand(command[0].c_str());
        if(close(new_fd)==-1)
        {
            Failed_syscall("close");
            return;
        }
        exit(0);
    }
    else if (pid>0)
    {
        if (waitpid(pid, NULL, WUNTRACED) == -1) {
            Failed_syscall("waitpid");
            return;
        }
    }
    else {
        Failed_syscall("fork");
    }
}

void ChmodCommand::execute() {
    if (argc != 3) {
        std::cerr << "smash error: chmod: invalid arguments" << std::endl;
        return;
    }

    int permi = atoi(argv[1]);
    if (permi < 0 || permi > 777) {
        std::cerr << "smash error: chmod: invalid arguments" << std::endl;
        return;
    }

    mode_t permissions = ((permi / 100) << 6) | (((permi / 10) % 10) << 3) | (permi % 10);
    if (chmod(argv[2], permissions) == -1) {
        Failed_syscall("chmod");
    }
}


void PipeCommand::execute() {
    int new_pipe[2];
    if(pipe(new_pipe)==-1)
    {
        Failed_syscall("pipe");
        return;
    }
    int fd;
    if(((string) cmd_line).find("|&") != std::string::npos)
    {
        fd=2;
    }
    else if (((string) cmd_line).find("|") != std::string::npos)
    {
        fd=1;
    }
    int std_fd = dup(0);
    pid_t pid = fork();
    if(pid == 0) {
        setpgrp();
        if (close(new_pipe[0]) == -1 || close(fd) == -1) {
            Failed_syscall("close");
            return;
        }
        if (dup2(new_pipe[1], fd) == -1) {
            Failed_syscall("dup2");
            return;
        }
        Command *command1 = SmallShell::getInstance().CreateCommand(command[0].c_str());
        if (command1) {
            command1->execute();
            delete command1;
        }
        exit(0);
    }
        else if(pid > 0)
        {
            if(close(new_pipe[1]) == -1 || close(0) == -1) {
                Failed_syscall("close");
                return;
            }
            if(dup2(new_pipe[0], 0) == -1) {
                Failed_syscall("dup2");
                return;
            }
            if (waitpid(pid, NULL, WUNTRACED) == -1) {
                Failed_syscall("waitpid");
                return;
            }
            Command *command2 = SmallShell::getInstance().CreateCommand(command[1].c_str());
            if (command2) {
                command2->execute();
                delete command2;
            }
        }

        else{
        Failed_syscall("fork");
        return;
        }

        if(close(0) == -1)
        {
            Failed_syscall("close");
            return;
        }

        if(dup2(std_fd,0) == -1)
        {
            Failed_syscall("dup2");
            return;
        }

    }




