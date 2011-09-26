#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include "Shared.h"
#include <getopt.h>

static int32_t locationLength = -1;
static int caseInsensitive = 0;

static int find(const void *l, const void *r)
{
    const char *left = ((const char*)l) + Int32Length;
    const char *right = ((const char*)r) + Int32Length;
    // printf("%s %s %s\n", left, right, std::string(left, locationLength).c_str());
    return (caseInsensitive
            ? strncasecmp(left, right, locationLength - Int32Length)
            : strncmp(left, right, locationLength - Int32Length));
}


void recurse(const char *ch, int32_t pos, int indent)
{
    struct NodeData node = readNodeData(ch + pos);
    int i;
    for (i=0; i<indent; ++i) {
        printf(" ");
    }
    printf("%s %s %s\n", nodeTypeToName(node.type, Normal), node.symbolName,
           node.location ? ch + node.location : "");
    if (node.firstChild)
        recurse(ch, node.firstChild, indent + 2);
    if (node.nextSibling)
        recurse(ch, node.nextSibling, indent);
}

static inline void usage(FILE *f)
{
    fprintf(f,
            "rc [options]...\n"
            "  --help|-h                  Display this help\n"
            "  --follow-symbol|-s [arg]   Follow this symbol (e.g. /tmp/main.cpp:32:1)\n"
            "  --references|-r [arg]      Print references of symbol at arg\n"
            "  --print-tree|-t            Print out the node tree to stdout\n"
            "  --list-symbols|-l [arg]    Print out symbols matching arg\n"
            "  --db-file|-f [arg]         Use this database file\n"
            "  --match-complete-symbol|-c Match only complete symbols (for --list-symbols)\n"
            "  --match-starts-with|-S     Match symbols that starts with the search term (for --list-symbols)\n"
            "  --case-insensitive|-i      Case insensitive matching\n");
}

int main(int argc, char **argv)
{
    struct option longOptions[] = {
        { "help", 0, 0, 'h' },
        { "follow-symbol", 1, 0, 's' },
        { "print-tree", 0, 0, 't' },
        { "db-file", 1, 0, 'f' },
        { "references", 1, 0, 'r' },
        { "list-symbols", 1, 0, 'l' },
        { "match-complete-symbol", 0, 0, 'c' },
        { "match-starts-with", 0, 0, 'S' },
        { "case-insensitive", 0, 0, 'i' },
        { 0, 0, 0, 0 },
    };
    const char *shortOptions = "hs:tf:r:l:cSi";
    int idx, longIndex;
    const char *arg = 0;
    const char *dbFile = 0;
    enum MatchType {
        MatchAnywhere,
        MatchStartsWith,
        MatchCompleteSymbol
    } matchType = MatchAnywhere;
    enum Mode {
        None,
        FollowSymbol,
        References,
        ListSymbols,
        ShowTree
    } mode = None;
    char dbFileBuffer[PATH_MAX + 10];
    // for (int i=0; i<argc; ++i) {
    //     printf("%d %s\n", i, argv[i]);
    // }

    while ((idx = getopt_long(argc, argv, shortOptions, longOptions, &longIndex)) != -1) {
        switch (idx) {
        case '?':
            usage(stderr);
            return 1;
        case 'i':
            caseInsensitive = 1;
            break;
        case 'h':
            usage(stdout);
            return 0;
        case 's':
            if (mode != None) {
                printf("%s %d: if (mode != None) {\n", __FILE__, __LINE__);
                return 1;
            }
            arg = optarg;
            mode = FollowSymbol;
            break;
        case 'r':
            arg = optarg;
            if (mode != None) {
                printf("%s %d: if (mode != None) {\n", __FILE__, __LINE__);
                return 1;
            }
            mode = References;
            break;
        case 't':
            if (mode != None) {
                printf("%s %d: if (mode != None) {\n", __FILE__, __LINE__);
                return 1;
            }
            mode = ShowTree;
            break;
        case 'f':
            dbFile = optarg;
            break;
        case 'l':
            if (mode != None) {
                printf("%s %d: if (mode != None) {\n", __FILE__, __LINE__);
                return 1;
            }
            mode = ListSymbols;
            arg = optarg;
            break;
        case 'S':
            if (matchType != MatchAnywhere) {
                printf("%s %d: if (matchType != MatchAnywhere) {\n", __FILE__, __LINE__);
                return 1;
            }
            matchType = MatchStartsWith;
            break;
        case 'c':
            if (matchType != MatchAnywhere) {
                printf("%s %d: if (matchType != MatchAnywhere) {\n", __FILE__, __LINE__);
                return 1;
            }
            matchType = MatchCompleteSymbol;
            break;
        }
    }
    if (matchType != MatchAnywhere && mode != ListSymbols) {
        printf("%s %d: if (matchType != MatchAnywhere && mode != ListSymbols)\n", __FILE__, __LINE__);
        return 1;
    }

    if (!dbFile) {
        if (!findDB(dbFileBuffer, sizeof(dbFileBuffer) - 1)) {
            printf("%s %d: if (!dbFile) {\n", __FILE__, __LINE__);
            return 1;
        } else {
            dbFile = dbFileBuffer;
        }
    }

    if (mode == None) {
        printf("%s %d: if (mode == None) {\n", __FILE__, __LINE__);
        return 1;
    }
        

    int fd = 0;
    struct stat st;
    void *mapped = 0;
    const char *ch = 0;

    fd = open(dbFile, O_RDONLY);
    if (fd <= 0) {
        printf("%s %d: if (fd <= 0)\n", __FILE__, __LINE__);
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        printf("%s %d: if (fstat(fdin, &st) < 0) \n", __FILE__, __LINE__);
        close(fd);
        return 1;
    }
    if (st.st_size < 10) {
        printf("%s %d: if (st.st_size < 10) {\n", __FILE__, __LINE__);
        close(fd);
        return 1;
    }
        
    if ((mapped = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        printf("%s %d: if ((src = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {\n", __FILE__, __LINE__);
        close(fd);
        return 1;
    }
    ch = (char*)mapped;
    if (strncmp(mapped, "Rt", 3)) {
        printf("%s %d: if (memcmp(mapped, \"Rt\", 2)) {\n", __FILE__, __LINE__);
        munmap(mapped, st.st_size);
        close(fd);
        return 1;
    }

    const int32_t nodeCount = readInt32(ch + NodeCountPos);
    locationLength = readInt32(ch + IdLengthPos);
    const int32_t dictionaryPosition = readInt32(ch + DictionaryPosPos);
    const int32_t dictionaryCount = readInt32(ch + DictionaryCountPos);
    /* printf("locationLength %d nodeCount %d\n" */
    /*        "dictionaryPosition %d dictionaryCount %d\n", locationLength, nodeCount, dictionaryPosition, dictionaryCount); */
    if (locationLength <= 0 || nodeCount <= 0) {
        munmap(mapped, st.st_size);
        close(fd);
        printf("%s %d: if (locationLength <= 0 || nodeCount <= 0)\n", __FILE__, __LINE__);
        return 1;
    }

    switch (mode) {
    case None:
        assert(0);
        break;
    case ShowTree:
        recurse(ch, rootNodePosition(nodeCount, locationLength), 0);
        break;
    case References:
    case FollowSymbol: {
        assert(arg);
        const int argLen = strlen(arg) + 1;
        char *padded = (char*)malloc(argLen + Int32Length);
        strncpy(padded + Int32Length, arg, argLen);
        const char *bs = (const char*)bsearch(padded, ch + FirstId, nodeCount, locationLength, find);
        // printf("Found a match %p\n", bs);
        free(padded);
        if (bs) {
            const int32_t idx = readInt32(bs);
            struct NodeData node = readNodeData(ch + idx);
            if (mode == References) {
                if (node.firstChild) {
                    node = readNodeData(ch + node.firstChild);
                    while (1) {
                        if (node.type == Reference && node.location) {
                            printf("%s\n", ch + node.location);
                        }
                        if (!node.nextSibling)
                            break;
                        node = readNodeData(ch + node.nextSibling);
                    }
                }
            } else {
                // printf("Found node %s %s\n", nodeTypeToName(type), symbolName);
                int32_t found = 0;
                NodeType targetType = MethodDeclaration;
                switch (node.type) {
                case MethodDeclaration:
                    targetType = MethodDefinition;
                case MethodDefinition: {
                    struct NodeData parent = readNodeData(ch + node.parent);
                    assert(parent.firstChild);
                    struct NodeData sibling = readNodeData(ch + parent.firstChild);
                    while (1) {
                        if (sibling.type == (int)targetType && !strcmp(node.symbolName, sibling.symbolName)) {
                            found = sibling.location;
                            break;
                        }
                        if (!sibling.nextSibling)
                            break;
                        sibling = readNodeData(ch + sibling.nextSibling);
                    }
                    break; }
                case Reference:
                case EnumValue:
                    found = readNodeData(ch + node.parent).location;
                    break;
                default:
                    break;
                }
                if (found) {
                    printf("%s\n", ch + found);
                } else {
                    printf("Couldn't find it\n");
                }
            }
        }
        break; }
    case ListSymbols: {
        int i;
        int32_t pos = dictionaryPosition;
        const int argLen = strlen(arg);
        for (i=0; i<dictionaryCount; ++i) {
            int32_t symbolName = pos;
            assert(ch[pos] > 32); // should be a printable character
            const int len = strlen(ch + pos);
            assert(len > 0);
            /* printf("Found symbol %s %d %d\n", ch + pos, len, pos); */
            int matched = 0;
            if (!argLen) {
                matched = 1;
            } else {
                switch (matchType) { // ### case-insensitive
                case MatchAnywhere:
                    if (caseInsensitive
                        ? strcasestr(ch + symbolName, arg)
                        : strstr(ch + symbolName, arg)) {
                        matched = 1;
                    }
                    break;
                case MatchCompleteSymbol:
                    if ((caseInsensitive
                         ? strcasecmp(ch + symbolName, arg)
                         : strcmp(ch + symbolName, arg)) == 0) {
                        matched = 1;
                    }
                    break;
                case MatchStartsWith:
                    if ((caseInsensitive
                         ? strncasecmp(ch + symbolName, arg, argLen)
                         : strncmp(ch + symbolName, arg, argLen)) == 0) {
                        matched = 1;
                    }
                    break;
                }
            }

            pos += len + 1;
            while (1) {
                int32_t loc = readInt32(ch + pos);
                pos += Int32Length;
                if (!loc)
                    break;
                if (matched)
                    printf("%s %s\n", ch + symbolName, ch + loc);
            }
        }
        break; }
    }
    munmap(mapped, st.st_size);
    close(fd);
    return 0;
}

