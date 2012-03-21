#include <proc/readproc.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <string>

struct Process
{
    int pid;
    int ppid;
    std::string name;
    std::string cmd;
    std::string user;

    unsigned int rss;
    unsigned int pss;
    unsigned int swap;
    unsigned int real;      // pss + swap

    bool operator<(const Process& v) const
    {
        return real > v.real; // inverted sort using a set
    }
};

struct Meminfo
{
    unsigned int rss;
    unsigned int pss;
    unsigned int swap;
    unsigned int real;

    Meminfo() : rss(0), pss(0), swap(0), real(0) {}
};

unsigned int ExtractSmapsValue(const std::string& line)
{
    std::istringstream ss(line);

    std::string label;
    ss >> label; // Skip label

    unsigned int value;
    ss >> value;

    return value;
}

void UpdateProcessMemory(Process& proc)
{
    std::ostringstream smapsPath;
    smapsPath << "/proc/" << proc.pid << "/smaps";
    std::ifstream smaps(smapsPath.str().c_str());
    if(!smaps.is_open())
        return;

    std::string line;
    while(std::getline(smaps, line))
    {
        if(line.find("Rss") == 0)
            proc.rss += ExtractSmapsValue(line);
        else if(line.find("Pss") == 0)
            proc.pss += ExtractSmapsValue(line);
        else if(line.find("Swap") == 0)
            proc.swap += ExtractSmapsValue(line);
    }

    proc.real = proc.pss + proc.swap;
}


void AdjustMinimumLength(const std::string& item, unsigned int& len)
{
    len = std::max((unsigned int)item.length(), len);
}

void AdjustMinimumLength(unsigned int item, unsigned int& len)
{
    std::ostringstream ss;
    ss << item;
    AdjustMinimumLength(ss.str(), len);
}

int main(int argc, char* argv[])
{
    typedef std::vector<Process> ProcessList;
    ProcessList processList;

    PROCTAB *proct;
    proc_t *proc_info;
    proct = openproc(PROC_FILLARG | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR);
    while ((proc_info = readproc(proct,NULL)))
    {
        Process proc;

        proc.pid = proc_info->tid;
        proc.ppid = proc_info->ppid;
        proc.name = proc_info->cmd;
        char** p = proc_info->cmdline;
        if(p)
        {
            while(*p)
            {
                proc.cmd += *p++;
                if(*p)
                    proc.cmd += " ";
            }
        }
        //p.cmd = *proc_info->cmdline;
        proc.user = proc_info->euser;

        proc.rss = proc.pss = proc.swap = 0;

        processList.push_back(proc);
    }
    closeproc(proct);


    unsigned int minsizepid = 0;
    unsigned int minsizeppid = 0;
    unsigned int minsizename = 0;
    unsigned int minsizeuser = 0;
    unsigned int minsizerss = 0;
    unsigned int minsizepss = 0;
    unsigned int minsizeswap = 0;
    unsigned int minsizereal = 0;
    for(unsigned int i = 0; i < processList.size(); ++i)
    {
        Process& proc = processList[i];
        UpdateProcessMemory(proc);
        //std::cout << proc.pid << ", " << proc.ppid << ", " << proc.name << ", " << proc.cmd << ", " << proc.user << std::endl;

        //std::cout << proc.pid << ", " << proc.ppid << ", " << proc.name << ", " << proc.rss << " kB, " << proc.user << std::endl;
        //std::cout << proc.pid << ", " << proc.ppid << ", " << proc.name << ", " << proc.rss << " kB, " << proc.user << std::endl;

        AdjustMinimumLength(proc.pid, minsizepid);
        AdjustMinimumLength(proc.ppid, minsizeppid);
        AdjustMinimumLength(proc.name, minsizename);
        AdjustMinimumLength(proc.user, minsizeuser);
        AdjustMinimumLength(proc.rss, minsizerss);
        AdjustMinimumLength(proc.pss, minsizepss);
        AdjustMinimumLength(proc.swap, minsizeswap);
        AdjustMinimumLength(proc.real, minsizereal);
    }

    minsizepid++;
    minsizeppid++;
    minsizename++;
    minsizeuser++;
    minsizerss++;
    minsizepss++;
    minsizeswap++;
    minsizereal++;



    if(argc == 2 && std::string(argv[1]) == "-u")
    {
        // Show memory per-user
        std::cout << std::setw(minsizeuser) << "USER";
        std::cout << std::setw(minsizerss) << "RSS";
        std::cout << std::setw(minsizepss) << "PSS";
        std::cout << std::setw(minsizeswap) << "SWAP";
        std::cout << std::setw(minsizereal) << "REAL";
        std::cout << std::endl;

        std::map<std::string, Meminfo> userMem;
        for(unsigned int i = 0; i < processList.size(); ++i)
        {
            const Process& proc = processList[i];

            userMem[proc.user].rss += proc.rss;
            userMem[proc.user].pss += proc.pss;
            userMem[proc.user].swap += proc.swap;
            userMem[proc.user].real += proc.real;
        }

        // Display
        for(std::map<std::string, Meminfo>::iterator it = userMem.begin(); it != userMem.end(); ++it)
        {
            const Meminfo& minfo = it->second;
            std::cout << std::setw(minsizeuser) << it->first;
            std::cout << std::setw(minsizerss) << minfo.rss;
            std::cout << std::setw(minsizepss) << minfo.pss;
            std::cout << std::setw(minsizeswap) << minfo.swap;
            std::cout << std::setw(minsizereal) << minfo.real;
            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << std::setw(minsizepid) << "PID";
        std::cout << std::setw(minsizeppid) << "PPID";
        std::cout << std::setw(minsizename) << "NAME";
        std::cout << std::setw(minsizeuser) << "USER";
        std::cout << std::setw(minsizerss) << "RSS";
        std::cout << std::setw(minsizepss) << "PSS";
        std::cout << std::setw(minsizeswap) << "SWAP";
        std::cout << std::setw(minsizereal) << "REAL";
        std::cout << std::endl;

        //Sort...
        std::multiset<Process> sortedList;
        for(unsigned int i = 0; i < processList.size(); ++i)
            sortedList.insert(processList[i]);

        // Display
        for(std::multiset<Process>::iterator it = sortedList.begin(); it != sortedList.end(); ++it)
        {
            const Process& proc = *it;
            std::cout << std::setw(minsizepid) << proc.pid;
            std::cout << std::setw(minsizeppid) << proc.ppid;
            std::cout << std::setw(minsizename) << proc.name;
            std::cout << std::setw(minsizeuser) << proc.user;
            std::cout << std::setw(minsizerss) << proc.rss;
            std::cout << std::setw(minsizepss) << proc.pss;
            std::cout << std::setw(minsizeswap) << proc.swap;
            std::cout << std::setw(minsizereal) << proc.real;
            std::cout << std::endl;
        }
    }
}
