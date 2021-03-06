#include <windows.h>
#include <pdh.h>
#include <stdio.h>
#include <stdlib.h>

BOOL WINAPI GetCounterValues(LPTSTR serverName, int n, char** instances);

int main(int argc, char *argv[])
{
    GetCounterValues(NULL, argc-1, &argv[1]);
}

BOOL WINAPI GetCounterValues(LPTSTR serverName, int instancesCount, char **instances)
{
    setbuf(stdout, NULL);
    
    PDH_STATUS s;

    HQUERY hQuery;

    // Array to specify the performance object, counter and instance for
    // which performance data should be collected.

    // typedef struct _PDH_COUNTER_PATH_ELEMENTS {
    //   LPTSTR  szMachineName;
    //   LPTSTR  szObjectName;
    //   LPTSTR  szInstanceName;
    //   LPTSTR  szParentInstance;
    //   DWORD   dwInstanceIndex;
    //   LPTSTR  szCounterName;
    // } PDH_COUNTER_PATH_ELEMENTS, *PPDH_COUNTER_PATH_ELEMENTS;

    // Each element in the array is a PDH_COUNTER_PATH_ELEMENTS structure.
    PDH_COUNTER_PATH_ELEMENTS cpeTmpl[] =
    {
        { NULL, "processor", "_total", NULL, -1, "% processor time"},
        { NULL, "system", NULL, NULL, -1, "processor queue length"},
        { NULL, "memory", NULL, NULL, -1, "available bytes"},
        { NULL, "memory", NULL, NULL, -1, "pages/sec"},
        { NULL, "physicaldisk", "_total", NULL, -1 , "% disk time"},
        { NULL, "asp.net applications", "__total__", NULL, -1, "requests/sec"},
        { NULL, "asp.net", NULL, NULL, -1, "application restarts"},
        { NULL, "asp.net", NULL, NULL, -1, "request wait time"},
        { NULL, "asp.net", NULL, NULL, -1, "requests queued"},
        { NULL, ".net clr exceptions", "_global_", NULL, -1, "# of exceps thrown / sec"},
        { NULL, ".net clr memory", "_global_", NULL, -1, "# total committed bytes"},
        { NULL, "web service", "_total", NULL, -1, "get requests/sec"},
        { NULL, "web service", "_total", NULL, -1, "post requests/sec"},
        { NULL, "web service", "_total", NULL, -1, "current connections"}
    };

    const int cpeTmplCount = sizeof(cpeTmpl) / sizeof(cpeTmpl[0]);
    const int countersCount = cpeTmplCount;
    int i, j;

    HCOUNTER *hCounter = malloc(sizeof(HCOUNTER) * countersCount);
    PDH_COUNTER_PATH_ELEMENTS *cpe = malloc(sizeof(PDH_COUNTER_PATH_ELEMENTS) * countersCount);

    char szFullPath[MAX_PATH];
    DWORD cbPathSize;

    int ret = -1;

    PDH_FMT_COUNTERVALUE counterValue;

    // Only do this setup once.
    if ((s = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
    {
        fprintf(stderr, "POQ failed %08x\n", s);
        return ret;
    }

    for (j = 0; j < cpeTmplCount; j++) {
        
        int index = i * cpeTmplCount + j;

        cbPathSize = sizeof(szFullPath);

        PDH_COUNTER_PATH_ELEMENTS cpeTmplItem = cpeTmpl[j];

        cpe[index].szMachineName = cpeTmplItem.szMachineName;
        cpe[index].szInstanceName = cpeTmplItem.szInstanceName;
        cpe[index].szObjectName = cpeTmplItem.szObjectName;
        cpe[index].szParentInstance = cpeTmplItem.szParentInstance;
        cpe[index].dwInstanceIndex = cpeTmplItem.dwInstanceIndex;
        cpe[index].szCounterName = cpeTmplItem.szCounterName;

        if ((s = PdhMakeCounterPath(&cpe[index],
            szFullPath, &cbPathSize, 0)) != ERROR_SUCCESS)
        {
            fprintf(stderr,"MCP failed %08x\n", s);
            return ret;
        }

        if ((s = PdhAddCounter(hQuery, szFullPath, 0, &hCounter[index]))
            != ERROR_SUCCESS)
        {
            fprintf(stderr, "PAC failed %08x for %s\n", s, cpe[index].szCounterName);
            //return ret;
        }
    }

    for (i = 0; i < 2; i++)
    {
        Sleep(100);

        // Collect data as often as you need to.
        if ((s = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
        {
            fprintf(stderr, "PCQD failed %08x\n", s);
            return ret;
        }

        if (i == 0) continue;

        // Extract the calculated performance counter value for each counter or instance.
        for (j = 0; j < countersCount; j++)
        {
            if ((s = PdhGetFormattedCounterValue(hCounter[j], PDH_FMT_DOUBLE,
                NULL, &counterValue)) != ERROR_SUCCESS)
            {
                fprintf(stderr, "PGFCV failed %08x %d\n", s, hCounter[j]);
                continue;
            }
            if (cpe[j].szInstanceName)
            {
                fprintf(stdout, "%s(%s)\\%s:%3.3f\n",
                    cpe[j].szObjectName,
                    cpe[j].szInstanceName,
                    cpe[j].szCounterName,
                    counterValue.doubleValue);
            }
            else
                fprintf(stdout, "%s\\%s:%3.3f\n",
                    cpe[j].szObjectName,
                    cpe[j].szCounterName,
                    counterValue.doubleValue);
        }
    }

    // Remove all the counters from the query.
    for (i = 0; i < countersCount; i++)
    {
        PdhRemoveCounter(hCounter[i]);
    }

    // Only do this cleanup once.
    PdhCloseQuery(hQuery);

    free(hCounter);
    free(cpe);

    return 0;
}
