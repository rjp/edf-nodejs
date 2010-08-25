#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

#include "EDF/EDF.h"

char json[1048576];

void recurse(EDF *tree, int root, int child, int depth)
{
    char *szType = NULL, *szMessage = NULL; //, *szEncoding = NULL;
    long lv; double dv;
    int loop = true;
    if (root) { tree->Root(); }
    if (child) { tree->Child(); }
    while (loop == true) {
        int t = tree->TypeGet(&szType, &szMessage, &lv, &dv);
        sprintf(json, "%s{ tag:\"%s\", value:\"", json, szType);
        printf("%*s%s ", depth, "", szType);
        switch (t) {
            case EDFElement::INT:
                printf("M=%ld\n", lv); break;
            case EDFElement::FLOAT:
                printf("M=%lf\n", dv); break;
            default:
                printf("M=%s\n", szMessage); break;
        }

        if (tree->Children() > 0) {
            recurse(tree, 0, 1, depth+1);
        }
        loop = tree->Next();
        if (loop == false) {
            tree->Parent();
        }
    }
}

int main(int argc, char **argv)
{
    char *szType = NULL, *szMessage = NULL; //, *szEncoding = NULL;
    int r;
    EDF *pTest = NULL;

    pTest = new EDF();
    r = pTest->Read("<edf=\"on\"><folder><mmh=44/></><test=\"cheese\"/><techno=42/><bigface><size=57/></></>", -1, 0);
    // r = pTest->Read("<edf=\"on\"/>", -1, 0);
    printf("Parsing result is %d\n", r);
    recurse(pTest, 1, 0, 0);
    printf("JSON:\n%s\n", json);
}

