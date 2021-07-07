#include "ResourcesParser/ResourceTypes.h"
#include "ResourcesParser/ResourcesParser.h"
#include "ResourcesParser/ResourcesParserInterpreter.h"

#include <iostream>
#include <sstream>

using namespace std;

int findArgvIndex(const char* argv, char *argvs[], int count);
const char* getArgv(const char* argv, char *argvs[], int count);
void printHelp();
void rebuild_arscfile(ResourcesParser parser);
void writeStringPool(FILE* pFile, ResourcesParser::ResStringPool* pStringPool);
void writePackageResource(FILE* pFile, ResourcesParser::PackageResource* pPkgRes);
void writeResTableType(FILE* pFile, ResourcesParser::ResTableType* pResTable);
void writeResTableUnknown(FILE* pFile, ResourcesParser::ResTableTypeUnknown* pResTableUnknown);

int main(int argc, char *argv[]) {
	const char* path = getArgv("-p", argv, argc);

	if(nullptr == path) {
		printHelp();
		return -1;
	} else if(getArgv("-h", argv, argc) != nullptr) {
		printHelp();
	}

	ResourcesParser parser(path);
    ResourcesParserInterpreter interpreter(&parser);
/*
	if(all >= 0) {
		interpreter.parserResource(ResourcesParserInterpreter::ALL_TYPE);
	}

	if(type) {
		interpreter.parserResource(type);
	}

	if(id) {
		interpreter.parserId(id);
	}
*/	

    //res/xml/network_security_config.xml
    
    uint32_t newResKeyId = parser.addResKeyStr("", "xml", "network_security_config");
    cout<<"[newResKeyId]:" << newResKeyId << endl;

    rebuild_arscfile(parser);

	return 0;
}


void rebuild_arscfile(ResourcesParser parser) {
    FILE* pFile = fopen("out.arsc", "w+");
    if (pFile == nullptr) {
       cout<<"pFile == nullptr, exit"<<endl;
       return;
    }

    // 写 ResTable_header
    fwrite(&(parser.mResourcesInfo), sizeof(ResTable_header), 1, pFile);

    // 写 Global String Pool 
    writeStringPool(pFile, parser.mGlobalStringPool.get());

    // 写 Table Package
    for (auto &item : parser.mResourceForPackageName) {
        writePackageResource(pFile, item.second.get()); 
    }

    fclose(pFile);

}

void writePackageResource(FILE* pFile, ResourcesParser::PackageResource* pPkgRes) {
    if (pPkgRes == nullptr) {
        return;
    }
    // write ResTable_package
    fwrite(&(pPkgRes->header), sizeof(ResTable_package), 1, pFile);

    // write ResType string pool
    writeStringPool(pFile, pPkgRes->pTypes.get());

    // write ResName string pool
    writeStringPool(pFile, pPkgRes->pKeys.get());

    int sizeFileCur = ftell(pFile);
    cout<<" ### [sizeFileCur]:0x"<<hex<<sizeFileCur<<dec<<endl;

    //
    for (auto &itemResTableUnknownPtr : pPkgRes->vecResTableUnknownPtrs) {
        writeResTableUnknown(pFile, itemResTableUnknownPtr.get());
    }

    //
    for (auto &itemKV : pPkgRes->resTablePtrs) {
        std::vector<ResourcesParser::ResTableTypePtr>& vectorResTable = itemKV.second;
        for (auto resTableItem : vectorResTable) {
            switch(resTableItem->header.header.type) {
                case RES_TABLE_TYPE_TYPE: {
                    writeResTableType(pFile, resTableItem.get());
                } break;
                default:
                break;
            }
        }
    }


}

void writeResTableType(FILE* pFile, ResourcesParser::ResTableType* pResTable) {
    if (pFile == nullptr || pResTable == nullptr) {
        return;
    }
    // 
    fwrite(&(pResTable->header), sizeof(ResTable_type), 1, pFile);

    // fill 28 space.
    uint32_t seek = pResTable->header.header.headerSize - sizeof(ResTable_type);
    if (seek > 0) {
        unsigned char* pBufTmp = new unsigned char[seek];
        memset(pBufTmp, 0, seek);
        fwrite(&pBufTmp, 1, seek, pFile);
        delete []pBufTmp;
        pBufTmp = nullptr;
    }

    // write entry offset array.
    const uint32_t sizeEntryOffset = sizeof(uint32_t) * pResTable->header.entryCount;
    fwrite(pResTable->entryPool.pOffsets.get(), sizeof(uint32_t), pResTable->header.entryCount, pFile);

    // fill space to dataStart
    const uint32_t sizeFill2DataStart = pResTable->header.entriesStart - pResTable->header.header.headerSize - sizeEntryOffset;
    if (sizeFill2DataStart > 0) {
        unsigned char* pBufTmp = new unsigned char[sizeFill2DataStart];
        memset(pBufTmp, 0, sizeFill2DataStart);
        fwrite(pBufTmp, 1, sizeFill2DataStart, pFile);
        delete []pBufTmp;
        pBufTmp = nullptr;
    }

    // write 
    fwrite(pResTable->entryPool.pData.get(), 1, pResTable->entryPool.dataSize, pFile);
}

void writeResTableUnknown(FILE* pFile, ResourcesParser::ResTableTypeUnknown* pResTableUnknown) {
    ResChunk_header* pChunkHeader = (ResChunk_header*)pResTableUnknown->pChunkAllData.get();
    
    fwrite(pResTableUnknown->pChunkAllData.get(), 1, pChunkHeader->size, pFile);
    
    //printHex((unsigned char*)pResTableUnknown->pChunkAllData.get(), sizeof(ResChunk_header));
}

void writeStringPool(FILE* pFile, ResourcesParser::ResStringPool* pStringPool) {
    // write ResStringPool_header
    fwrite(&(pStringPool->header), sizeof(ResStringPool_header), 1, pFile);

    // write String offset array
    if (pStringPool->header.stringCount > 0) {
        const uint32_t sizeStrOffset = sizeof(uint32_t) * pStringPool->header.stringCount;
        fwrite(pStringPool->pOffsets.get(), 1, sizeStrOffset, pFile);
    }
    //
    if (pStringPool->header.styleCount > 0) {
        const uint32_t sizeStyleOffset = sizeof(uint32_t) * pStringPool->header.styleCount;
        fwrite(pStringPool->pStyleOffsets.get(), 1, sizeStyleOffset, pFile);
    }
    // write string array.
    if (pStringPool->header.stringCount > 0) {
        const uint32_t strBufSize = pStringPool->header.styleCount > 0
            ? pStringPool->header.stylesStart - pStringPool->header.stringsStart
            : pStringPool->header.header.size - pStringPool->header.stringsStart;
        fwrite(pStringPool->pStrings.get(), 1, strBufSize, pFile);
    }
    // write style array.
    if (pStringPool->header.styleCount > 0) {
        const uint32_t styleBufSize = pStringPool->header.header.size - pStringPool->header.stylesStart;
        fwrite(pStringPool->pStyles.get(), 1, styleBufSize, pFile);
    }

}

int findArgvIndex(const char* argv, char *argvs[], int count) {
	for(int i = 0 ; i<count ; i++) {
		if(strcmp(argv, argvs[i])==0) {
			return i;
		}
	}
	return -1;
}

const char* getArgv(const char* argv, char *argvs[], int count) {
	int index = findArgvIndex(argv, argvs, count);
	if(index>=0 && index+1 < count) {
		return argvs[index+1];
	}
	return nullptr;
}

void printHelp() {
	cout <<"rp -p path [-a] [-t type] [-i id]" <<endl<<endl;
	cout <<"-p : set path of resources.arsc" <<endl;
	cout <<"-a : show all of resources.arsc" <<endl;
	cout <<"-t : select the type in resources.arsc to show" <<endl;
	cout <<"-i : select the id of resource to show" <<endl;
}
