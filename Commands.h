#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <fcntl.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


int _parseCommandLine(const char *cmd_line, char **args);

void Failed_syscall(const char *syscall);

std::string _trim(const std::string &s);




class Command {
// TODO: Add your data members

 public:

    const char *cmd_line;
    int argc;
    char *argv[COMMAND_MAX_ARGS];

    Command(const char* cmd_line) : cmd_line(cmd_line),argc(_parseCommandLine(cmd_line,argv)) {}
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
    void removeBackgroundSign();

};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line) : Command(cmd_line)
  {
      if (argc != 0 && std::string(argv[argc - 1]).compare("&") == 0) {
          argv[argc - 1] = nullptr;
          argc--;
      }
  }
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
    std::string command[2];

    PipeCommand(const char* cmd_line) : Command(cmd_line)
    {
        command[0] = ((std::string) cmd_line).substr(0, ((std::string) cmd_line).find('|'));
        if (std::string(command[0]).end().operator*() == '|') {
            std::string(command[0]).pop_back();
        }
        command[1] = ((std::string) cmd_line).substr(((std::string) cmd_line).find_last_of('|'));
        if (std::string(command[1]).begin().operator*() == '|') {
            command[1] = std::string(command[1]).substr(1, std::string(command[1]).size() - 1);

        }
        if (std::string(command[1]).begin().operator*() == '&') {
            command[1] = std::string(command[1]).substr(1, std::string(command[1]).size() - 1);
        }
        command[1] = _trim(command[1]);
    }
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
    std::string command[2];
  explicit RedirectionCommand(const char* cmd_line) : Command(cmd_line)
  {
      command[0] = ((std::string) cmd_line).substr(0, ((std::string) cmd_line).find('>'));
      if (std::string(command[0]).end().operator*() == '>') {
          std::string(command[0]).pop_back();
      }

      command[1] = ((std::string) cmd_line).substr(((std::string) cmd_line).find_last_of('>'));
      if (std::string(command[1]).begin().operator*() == '>') {
          command[1] = std::string(command[1]).substr(1, std::string(command[1]).size() - 1);

      }
      command[1] = _trim(command[1]);
  }

  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class chprompt : public BuiltInCommand{
public:
    chprompt(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~chprompt() {}
    void execute() override;
};


class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
  public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line) {}
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
  JobsList* jobs;
  QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobs(jobs){}
  virtual ~QuitCommand() {}
  void execute() override;
};




class JobsList {
 public:
  class JobEntry {
  public:
      int id;
      time_t time;
      pid_t pid;
      std::string cmd;

      JobEntry(int id, time_t time, pid_t pid, const std::string &cmd):
      id(id),
      time(time),
      pid(pid),
      cmd(cmd) {}

      ~JobEntry() = default;

      bool operator<(const JobEntry &other_job)
      {
          return this->id < other_job.id;
      }
      bool operator>(const JobEntry &other_job)
      {
          return this->id > other_job.id;
      }
      bool operator<=(const JobEntry &other_job)
      {
          return this->id <= other_job.id;
      }
      bool operator>=(const JobEntry &other_job)
      {
          return this->id >= other_job.id;
      }
      bool operator==(const JobEntry &other_job)
      {
          return this->id == other_job.id;
      }
  };
 public:
    std::vector<JobEntry*> jobs_vec;

  JobsList() = default;
  ~JobsList() = default;
  void addJob(std::string cmd_line,pid_t pid);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  JobEntry * getJobByPId(pid_t pid);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  JobEntry *maxJobId();
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {}
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
    JobsList *jobs;
    KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobs(jobs){}
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ChmodCommand() {}
  void execute() override;
};


class SmallShell {
 private:

  // TODO: Add your data members
    SmallShell();
 public:
    std::string prompt;
    pid_t pid;
    int cnt = 0;
    std::string last_dir;
    JobsList* jobs_list;
    pid_t current_jobpid=0;
    std::string current_jobcmdline= "";
    Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
