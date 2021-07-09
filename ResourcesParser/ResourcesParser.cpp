#include "ResourcesParser.h"

#include <codecvt>
#include <locale>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

#define RETURN_UNKNOWN_ID(ID) stringstream ss; \
	ss <<"???\(0x" <<hex <<setw(8) <<setfill('0') <<ID <<")"; \
	return ss.str()

using namespace std;

static string stringOfComplex(uint32_t complex, bool isFraction) {
	stringstream ss;
    const float MANTISSA_MULT =
        1.0f / (1<<Res_value::COMPLEX_MANTISSA_SHIFT);
    const float RADIX_MULTS[] = {
        1.0f*MANTISSA_MULT, 1.0f/(1<<7)*MANTISSA_MULT,
        1.0f/(1<<15)*MANTISSA_MULT, 1.0f/(1<<23)*MANTISSA_MULT
    };

    float value = (complex&(Res_value::COMPLEX_MANTISSA_MASK
                   <<Res_value::COMPLEX_MANTISSA_SHIFT))
            * RADIX_MULTS[(complex>>Res_value::COMPLEX_RADIX_SHIFT)
                            & Res_value::COMPLEX_RADIX_MASK];
	ss<<value;

    if (!isFraction) {
        switch ((complex>>Res_value::COMPLEX_UNIT_SHIFT)&Res_value::COMPLEX_UNIT_MASK) {
            case Res_value::COMPLEX_UNIT_PX: ss<<"px"; break;
            case Res_value::COMPLEX_UNIT_DIP: ss<<"dp"; break;
            case Res_value::COMPLEX_UNIT_SP: ss<<"sp"; break;
            case Res_value::COMPLEX_UNIT_PT: ss<<"pt"; break;
            case Res_value::COMPLEX_UNIT_IN: ss<<"in"; break;
            case Res_value::COMPLEX_UNIT_MM: ss<<"mm"; break;
            default: ss<<" (unknown unit)"; break;
        }
    } else {
        switch ((complex>>Res_value::COMPLEX_UNIT_SHIFT)&Res_value::COMPLEX_UNIT_MASK) {
            case Res_value::COMPLEX_UNIT_FRACTION: ss<<"%"; break;
            case Res_value::COMPLEX_UNIT_FRACTION_PARENT: ss<<"%p"; break;
            default: ss<<" (unknown unit)"; break;
        }
    }
	return ss.str();
}

string ResourcesParser::stringOfValue(const Res_value* value) const {
	stringstream ss;
    if (value->dataType == Res_value::TYPE_NULL) {
        ss<<"(null)";
    } else if (value->dataType == Res_value::TYPE_REFERENCE) {
		ss<<"(reference) "<<getNameForId(value->data);
    } else if (value->dataType == Res_value::TYPE_ATTRIBUTE) {
		ss<<"(attribute) "<<getNameForId(value->data);
    } else if (value->dataType == Res_value::TYPE_STRING) {
		ss<<"(string) "<<getStringFromGlobalStringPool(value->data);
    } else if (value->dataType == Res_value::TYPE_FLOAT) {
        ss<<"(float) "<<*(const float*)&value->data;
    } else if (value->dataType == Res_value::TYPE_DIMENSION) {
        ss<<"(dimension) "<<stringOfComplex(value->data, false);
    } else if (value->dataType == Res_value::TYPE_FRACTION) {
        ss<<"(fraction) "<<stringOfComplex(value->data, true);
    } else if (value->dataType >= Res_value::TYPE_FIRST_COLOR_INT
            && value->dataType <= Res_value::TYPE_LAST_COLOR_INT) {
		ss<<"(color) #"<<hex<<setw(8)<<setfill('0')<<value->data;
    } else if (value->dataType == Res_value::TYPE_INT_BOOLEAN) {
        ss<<"(boolean) "<<(value->data ? "true" : "false");
    } else if (value->dataType >= Res_value::TYPE_FIRST_INT
            && value->dataType <= Res_value::TYPE_LAST_INT) {
        ss<<"(int) "<<value->data<<" or 0x"<<hex<<setw(8)<<setfill('0')<<value->data;
    } else {
		ss<<"(unknown type) "
			<<"t=0x"<<hex<<setw(2)<<setfill('0')<<(int)value->dataType<<" "
			<<"d=0x"<<hex<<setw(8)<<setfill('0')<<(int)value->data<<" "
			<<"(s=0x"<<hex<<setw(4)<<setfill('0')<<(int)value->size<<" "
			<<"r=0x"<<hex<<setw(2)<<setfill('0')<<(int)value->res0<<")";
    }
	return ss.str();
}

inline static string toUtf8(const u16string& str16) {
	return wstring_convert<codecvt_utf8_utf16<char16_t>,char16_t>()
		.to_bytes(str16);
}

inline static u16string toUtf16(const string& strUtf8) {
    return wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t>()
        .from_bytes(strUtf8);
}

ResourcesParser::ResourcesParser(const string& filePath) {
	ifstream resources(filePath, ios::in|ios::binary);

	// resources文件开头是个ResTable_header,记录整个文件的信息
	resources.read((char*)&mResourcesInfo, sizeof(ResTable_header));
	//printHex((unsigned char*)&mResourcesInfo, sizeof(ResTable_header));

	// 紧接着就是全局字符串池
	mGlobalStringPool = parserResStringPool(resources);
	if(mGlobalStringPool == nullptr) {
		return;
	}

	for(int i = 0 ; i < mResourcesInfo.packageCount ; i++) {
		PackageResourcePtr pResource = parserPackageResource(resources);
		if(pResource == nullptr) {
			return;
		}
		mResourceForId[pResource->header.id] = pResource;
		mResourceForPackageName[toUtf8((char16_t*)pResource->header.name)] = pResource;
        cout<<"[ResHeaderName]: "<<toUtf8((char16_t*)pResource->header.name)<<endl;
	}
}

void printHex(unsigned char* pBuf, unsigned int uLenBuf) {
    if (pBuf == nullptr) {
        cout<<"[printHex] NULL";
        return;
    }
    if (uLenBuf == 0) {
        cout<<"[printHex] []"; 
    	return;
    }
    cout<<"[printHex] "<<uLenBuf<<endl;
    char* pMsg = new char[uLenBuf * 3 + 2]; 
    char* pPos = pMsg;
    unsigned char ucTmp = 0;
    for (unsigned int uIdx = 0; uIdx<uLenBuf; ++uIdx) {
        ucTmp = pBuf[uIdx];
        pPos[uIdx * 3] = ((ucTmp>>4) < 10) ? ('0' + (ucTmp>>4)) : ('a' + (ucTmp>>4) - 10);
        pPos[uIdx * 3 + 1] = ((ucTmp&0x0F) < 10) ? ('0' + (ucTmp&0x0F)) : ('a' + (ucTmp&0x0F) - 10);
        pPos[uIdx * 3 + 2] = ' ';
    }
    pMsg[uLenBuf * 3] = 0;
    cout<<"[Hex]:"<<pMsg<<endl;
    delete[] pMsg;
    pMsg = nullptr;
}

void printChunkHeader(ResChunk_header* pChunkHeader) {
    if (pChunkHeader == nullptr) {
        cout<<"pChunkHeader == nullptr"<<endl;
	return;
    }
    cout<<"["<<pChunkHeader->type<<"]["<<pChunkHeader->headerSize<<"]["<<pChunkHeader->size<<"]"<<endl;

}

ResourcesParser::ResStringPoolPtr ResourcesParser::parserResStringPool(
		ifstream& resources) {
    int32_t uCur = resources.tellg();
    cout<<"[StringPoll] start: 0x"<<hex<<uCur<<dec<<endl;
    
	ResStringPoolPtr pPool = make_shared<ResStringPool>();
	resources.read((char*)&pPool->header, sizeof(ResStringPool_header));
    //printHex((unsigned char*)&(pPool->header), sizeof(ResStringPool_header));
	printChunkHeader(&pPool->header.header);
	if(pPool->header.header.type != RES_STRING_POOL_TYPE) {
		cout<<"["<<pPool->header.header.type<<"]parserResStringPool 需要定位到 RES_STRING_POOL_TYPE !"<<endl;
		return nullptr;
	}
    cout<<"stringCnt:"<<pPool->header.stringCount<<endl;
    cout<<"styleCnt:"<<pPool->header.styleCount<<endl;
	cout<<"stringStart:"<<pPool->header.stringsStart<<endl;
	cout<<"stylesStart:"<<pPool->header.stylesStart<<endl;

	pPool->pOffsets = shared_ptr<uint32_t>(
			new uint32_t[pPool->header.stringCount],
			default_delete<uint32_t[]>()
	);

	const uint32_t offsetSize = sizeof(uint32_t) * pPool->header.stringCount;
	resources.read((char*)pPool->pOffsets.get(), offsetSize);

	// 跳到字符串数组开头位置
	uint32_t seek = pPool->header.stringsStart
		- pPool->header.header.headerSize
		- offsetSize;
	resources.seekg(seek, ios::cur);

    if (seek == 0) {
        const uint32_t styleOffsetSize = sizeof(uint32_t) * pPool->header.styleCount;
        cout<<"[seek]:"<<seek<<", [styleOffsetSize]:"<<styleOffsetSize<<endl;
    }

	// 载入所有字符串
	const uint32_t strBuffSize = pPool->header.styleCount > 0
		? pPool->header.stylesStart - pPool->header.stringsStart
		: pPool->header.header.size - pPool->header.stringsStart;
	pPool->pStrings = shared_ptr<byte>(
			new byte[strBuffSize],
			default_delete<byte[]>()
	);
	resources.read((char*)pPool->pStrings.get(), strBuffSize);

	// 跳出全局字符串池
	if(pPool->header.styleCount > 0) {
		seek = pPool->header.header.size
			- pPool->header.stringsStart
			- strBuffSize;
		resources.seekg(seek, ios::cur);
	}

	return pPool;
}

string ResourcesParser::getStringFromGlobalStringPool(uint32_t index) const {
	return getStringFromResStringPool(mGlobalStringPool, index);
}

string ResourcesParser::getStringFromResStringPool(
			ResourcesParser::ResStringPoolPtr pPool,
			uint32_t index) {
	if(index > pPool->header.stringCount) {
		return "???";
	}
	uint32_t offset = *(pPool->pOffsets.get() + index);
	//前两个字节是字符串长度
	char* str = ((char*) (pPool->pStrings.get() + offset + 2));
	return (pPool->header.flags & ResStringPool_header::UTF8_FLAG)
		? str
		: toUtf8((char16_t*) str);
}

void ResourcesParser::printResStrPool(ResStringPoolPtr pResStrPool) {
    if (pResStrPool == nullptr) {
        return; 
    }
    cout<<endl;
    cout<<"[stringCount]:"<<pResStrPool->header.stringCount<<endl;
    cout<<endl;

    std::string itemStr = "";
    for (int idx = 0; idx<pResStrPool->header.stringCount; ++idx) {
        itemStr = getStringFromResStringPool(pResStrPool, idx); 
	cout<< itemStr <<endl;
    } 
    cout<<endl;

}

ResourcesParser::PackageResourcePtr ResourcesParser::parserPackageResource(
		ifstream& resources) {
	PackageResourcePtr pPool = make_shared<PackageResource>();
	resources.read((char*)&pPool->header, sizeof(ResTable_package));

	if(pPool->header.header.type != RES_TABLE_PACKAGE_TYPE) {
		cout<<"parserPackageResource 需要定位到 RES_TABLE_PACKAGE_TYPE !"<<endl;
		return nullptr;
	}

	// 接着是资源类型字符串池
	pPool->pTypes = parserResStringPool(resources);
    printResStrPool(pPool->pTypes);

    cout<< "############################" <<endl<<endl;

	// 接着是资源名称字符串池
	pPool->pKeys = parserResStringPool(resources);
//	printResStrPool(pPool->pKeys);

	ResChunk_header chunkHeader;
	while(resources.read((char*)&chunkHeader, sizeof(ResChunk_header))) {
		resources.seekg(-sizeof(ResChunk_header), ios::cur);

		if(chunkHeader.type == RES_TABLE_PACKAGE_TYPE) {
			return pPool;
		} else if(chunkHeader.type == RES_TABLE_TYPE_TYPE) {
			ResTableTypePtr pResTableType = make_shared<ResTableType>();
			resources.read((char*)&pResTableType->header, sizeof(ResTable_type));
//            cout<<"[after read ResTableType][0x"<<hex<<resources.tellg()<<"]["<<dec<<pResTableType->header.header.headerSize<<"]"<<endl;
            //
			uint32_t seek = pResTableType->header.header.headerSize - sizeof(ResTable_type);
			resources.seekg(seek, ios::cur);
            //cout<<"[seek]#####"<<dec<<seek<<endl;
//            cout<<"[0x"<<hex<<chunkHeader.type<<"][0x"<<resources.tellg()<<"][ResTableTypeId]:"<<dec<<(unsigned int)pResTableType->header.id<<", [EntryCount]:"<<pResTableType->header.entryCount<<", [EntriesStart]:"<<pResTableType->header.entriesStart<<", [config]:"<<pResTableType->header.config.toString()<<endl;

			pResTableType->entryPool = parserEntryPool(
					resources,
					pResTableType->header.entryCount,
					pResTableType->header.entriesStart - pResTableType->header.header.headerSize,
					pResTableType->header.header.size - pResTableType->header.entriesStart);
			pPool->resTablePtrs[pResTableType->header.id].push_back(pResTableType);
			for(int i = 0 ; i < pResTableType->header.entryCount ; i++) {
				ResTable_entry* pEntry = getEntryFromEntryPool(pResTableType->entryPool, i);
				if(nullptr == pEntry) {
					pResTableType->entries.push_back(nullptr);
					pResTableType->values.push_back(nullptr);
					continue;
				}
				Res_value* pValue = getValueFromEntry(pEntry);
				pResTableType->entries.push_back(pEntry);
				pResTableType->values.push_back(pValue);
			//	cout<<"["<<pEntry->key.index<<"]"<<getStringFromResStringPool(pPool->pKeys,pEntry->key.index)<<endl;
			}
		} else {
            cout<<"[0x"<<hex<<chunkHeader.type<<"] size:0x"<<chunkHeader.size<<dec<<endl;
//			resources.seekg(chunkHeader.size, ios::cur);
			ResTableTypeUnknownPtr pResTableTypeUnknownPtr = make_shared<ResTableTypeUnknown>();
            pResTableTypeUnknownPtr->pChunkAllData = shared_ptr<byte>(
			    new byte[chunkHeader.size],
                default_delete<byte[]>()
            );
			resources.read((char*)pResTableTypeUnknownPtr->pChunkAllData.get(), chunkHeader.size);
            pPool->vecResTableUnknownPtrs.push_back(pResTableTypeUnknownPtr);

            //printHex((unsigned char*)pResTableTypeUnknownPtr->pChunkAllData.get(), sizeof(ResChunk_header));
            
		}
	}

	return pPool;
}

ResourcesParser::EntryPool ResourcesParser::parserEntryPool(
			ifstream& resources,
			uint32_t entryCount,
			uint32_t dataStart,
			uint32_t dataSize) {
	EntryPool pool;
	pool.pOffsets = shared_ptr<uint32_t>(
			new uint32_t[entryCount],
			default_delete<uint32_t[]>()
	);

	const uint32_t offsetSize = sizeof(uint32_t) * entryCount;
	resources.read((char*)pool.pOffsets.get(), offsetSize);

	pool.offsetCount = entryCount;
	pool.dataSize = dataSize;

	resources.seekg(dataStart - offsetSize, ios::cur);

	pool.pData = shared_ptr<byte>(
			new byte[pool.dataSize],
			default_delete<byte[]>()
	);
	resources.read((char*)pool.pData.get(), pool.dataSize);
	return pool;
}

ResTable_entry* ResourcesParser::getEntryFromEntryPool(EntryPool pool, uint32_t index) {
	if(index >= pool.offsetCount) {
		return nullptr;
	}
	uint32_t offset = *(pool.pOffsets.get() + index);
	if(offset == ResTable_type::NO_ENTRY) {
		return nullptr;
	}
	return ((ResTable_entry*)(pool.pData.get() + offset));
}

Res_value* ResourcesParser::getValueFromEntry(const ResTable_entry* pEntry) {
	return (Res_value*)(((byte*)pEntry) + pEntry->size);
}

ResTable_map* ResourcesParser::getMapsFromEntry(const ResTable_entry* pEntry) {
	return (ResTable_map*)(((byte*)pEntry) + pEntry->size);
}


ResourcesParser::PackageResourcePtr ResourcesParser::getPackageResouceForId(uint32_t id) const {
	uint32_t packageId = (id >> 24);
	auto it = mResourceForId.find(packageId);
	if(it==mResourceForId.end()) {
		return nullptr;
	}
	return it->second;
}

vector<ResourcesParser::ResTableTypePtr> ResourcesParser::getResTableTypesForId(uint32_t id) {
	PackageResourcePtr pPackage = getPackageResouceForId(id);
	if(pPackage == nullptr) {
		return vector<ResTableTypePtr>();
	}
	return pPackage->resTablePtrs[TYPE_ID(id)];
}

string ResourcesParser::getNameForId(uint32_t id) const {
	PackageResourcePtr pPackage = getPackageResouceForId(id);
	if(pPackage == nullptr) {
		RETURN_UNKNOWN_ID(id);
	}
	uint32_t typeId = TYPE_ID(id);
	if(pPackage->resTablePtrs.find(typeId)==pPackage->resTablePtrs.end()) {
		RETURN_UNKNOWN_ID(id);
	}

	uint32_t entryId = ENTRY_ID(id);

	if(pPackage->resTablePtrs[typeId][0]->header.entryCount<=(entryId)){
		RETURN_UNKNOWN_ID(id);
	}

	const ResTable_entry* pEntry = nullptr;
	for(ResTableTypePtr  pResTableType : pPackage->resTablePtrs[typeId]) {
		pEntry = pResTableType->entries[entryId];
		if(pEntry) {
			break;
		}
	}

	if(pEntry == nullptr) {
		RETURN_UNKNOWN_ID(id);
	}
	return getStringFromResStringPool(getPackageResouceForId(id)->pKeys, pEntry->key.index);
}

string ResourcesParser::getNameForResTableMap(const ResTable_ref& ref) const {
	switch(ref.ident) {
		case ResTable_map::ATTR_TYPE:
			return "ATTR_TYPE";
        case ResTable_map::ATTR_MIN:
			return "ATTR_MIN";
		case ResTable_map::ATTR_MAX:
			return "ATTR_MAX";
		case ResTable_map::ATTR_L10N:
			return "ATTR_L10N";
       	case ResTable_map::ATTR_OTHER:
			return "ATTR_OTHER";
       	case ResTable_map::ATTR_ZERO:
			return "ATTR_ZERO";
       	case ResTable_map::ATTR_ONE:
			return "ATTR_ONE";
       	case ResTable_map::ATTR_TWO:
			return "ATTR_TWO";
       	case ResTable_map::ATTR_FEW:
			return "ATTR_FEW";
       	case ResTable_map::ATTR_MANY:
			return "ATTR_MANY";
		default:
			return getNameForId(ref.ident);
	}
}

bool ResourcesParser::isTableMapForAttrDesc(const ResTable_ref& ref) {
	switch(ref.ident) {
		case ResTable_map::ATTR_TYPE:
        case ResTable_map::ATTR_MIN:
		case ResTable_map::ATTR_MAX:
		case ResTable_map::ATTR_L10N:
       	case ResTable_map::ATTR_OTHER:
       	case ResTable_map::ATTR_ZERO:
       	case ResTable_map::ATTR_ONE:
       	case ResTable_map::ATTR_TWO:
       	case ResTable_map::ATTR_FEW:
       	case ResTable_map::ATTR_MANY:
			return true;
		default:
			return false;
	}
}

string ResourcesParser::getValueTypeForResTableMap(const Res_value& value) const {
	switch(value.data) {
		case ResTable_map::TYPE_ANY:
			return "any";
		case ResTable_map::TYPE_REFERENCE:
			return "reference";
		case ResTable_map::TYPE_STRING:
			return "string";
		case ResTable_map::TYPE_INTEGER:
			return "integer";
		case ResTable_map::TYPE_BOOLEAN:
			return "boolean";
		case ResTable_map::TYPE_COLOR:
			return "color";
		case ResTable_map::TYPE_FLOAT:
			return "float";
		case ResTable_map::TYPE_DIMENSION:
			return "dimension";
		case ResTable_map::TYPE_FRACTION:
			return "fraction";
		case ResTable_map::TYPE_ENUM:
			return "enum";
		case ResTable_map::TYPE_FLAGS:
			return "flags";
		default:
			return "unknown";
	}
}

uint32_t ResourcesParser::ResStringPool::addNewString_new(std::string& newStr) {
    uint32_t uTotalAdd = 0;

    // 开始增加字符串长度和内容
    const uint32_t sizeStrBufOrigin = header.styleCount > 0
        ? header.stylesStart - header.stringsStart
        : header.header.size - header.stringsStart;
    uint8_t uLenStrNew = newStr.size(); // 无论 utf8 还是 utf16， 这里记录的是字符数，不包括最后的 0 结束符.
    cout<<"[ROM_DEBUG] len:"<<newStr.length()<<", size:"<<newStr.size()<<endl;
    uint32_t uSizeNewStrBufAdd = 0;
    if (header.flags & ResStringPool_header::UTF8_FLAG) {
        uSizeNewStrBufAdd = 2 + uLenStrNew + 1; 
        uint32_t uSizeBufNew = uSizeNewStrBufAdd + sizeStrBufOrigin; 
        shared_ptr<byte> pBufStrDataNew = shared_ptr<byte>(new byte[uSizeBufNew], default_delete<byte[]>());
        memset(pBufStrDataNew.get(), 0, uSizeBufNew);
        memcpy(pBufStrDataNew.get(), &uLenStrNew, 1);
        memcpy(pBufStrDataNew.get() + 1, &uLenStrNew, 1);
        memcpy(pBufStrDataNew.get() + 2, newStr.c_str(), uLenStrNew + 1);
        memcpy(pBufStrDataNew.get() + uSizeNewStrBufAdd, pStrings.get(), sizeStrBufOrigin); //把老数据拷贝过去.
        pStrings.swap(pBufStrDataNew);
        //
        uTotalAdd += uSizeNewStrBufAdd;
    } else {
        u16string newStr16 = toUtf16(newStr);
        uSizeNewStrBufAdd = 2 + (uLenStrNew + 1)*2; 
        uint16_t uSizeBufNew = uSizeNewStrBufAdd + sizeStrBufOrigin;
        shared_ptr<byte> pBufStrDataNew = shared_ptr<byte>(new byte[uSizeBufNew], default_delete<byte[]>());
        memset(pBufStrDataNew.get(), 0, uSizeBufNew);
        memcpy(pBufStrDataNew.get(), &uLenStrNew, 1);
        memcpy(pBufStrDataNew.get() + 1, &uLenStrNew, 1);
        memcpy(pBufStrDataNew.get() + 2, newStr.c_str(), (uLenStrNew + 1)*2);
        memcpy(pBufStrDataNew.get() + uSizeNewStrBufAdd, pStrings.get(), sizeStrBufOrigin); //把老数据拷贝过去.
        pStrings.swap(pBufStrDataNew);
        //
        uTotalAdd += uSizeNewStrBufAdd;
    }
    
    // add new string offset
    std::shared_ptr<uint32_t> pOffsetsNew = shared_ptr<uint32_t>(
        new uint32_t[header.stringCount+1],
        default_delete<uint32_t[]>()
    );
    *(pOffsetsNew.get()) = header.stringsStart + sizeof(uint32_t);
    for (uint32_t idx = 0; idx<header.stringCount; ++idx) {
       *(pOffsetsNew.get() + 1 + idx) = *(pOffsets.get() + idx) + sizeof(uint32_t) + uSizeNewStrBufAdd;
    }
    // 交换内存
    pOffsets.swap(pOffsetsNew);
    // 记录 offset 新增内存大小
    uTotalAdd += sizeof(uint32_t);

    // 字符串统计+1
    header.stringCount += 1; 
    // update meta data.
    header.stringsStart += sizeof(uint32_t);
    // 调码整 style起始位置, style偏移数组里面的值. 
    if (header.stylesStart > 0) {
        header.stylesStart += uTotalAdd;
    }
    
    // 整体字符串包大小也要改.
    header.header.size += uTotalAdd;

    return uTotalAdd;
}

uint32_t ResourcesParser::ResStringPool::addNewString(std::string& newStr) {
    uint32_t uTotalAdd = 0;
    std::shared_ptr<uint32_t> pOffsetsNew = shared_ptr<uint32_t>(
        new uint32_t[header.stringCount+1],
        default_delete<uint32_t[]>()
    );
    // add new string offset)
    memcpy(pOffsetsNew.get(), pOffsets.get(), header.stringCount*sizeof(uint32_t));
    uint32_t newStringStart = (header.styleCount > 0)
        ? (header.stylesStart - header.stringsStart)
        : (header.header.size - header.stringsStart);
    *(pOffsetsNew.get() + header.stringCount) = newStringStart; 
    // 交换内存
    pOffsets.swap(pOffsetsNew);

    // 记录 offset 新增内存大小
    uTotalAdd += sizeof(uint32_t);

    // 开始增加字符串长度和内容
    const uint32_t sizeStrBufOrigin = header.styleCount > 0
        ? header.stylesStart - header.stringsStart
        : header.header.size - header.stringsStart;
    uint8_t uLenStrNew = newStr.size(); // 无论 utf8 还是 utf16， 这里记录的是字符数，不包括最后的 0 结束符.
    cout<<"[ROM_DEBUG] len:"<<newStr.length()<<", size:"<<newStr.size()<<endl;
    if (header.flags & ResStringPool_header::UTF8_FLAG) {
        uint32_t uSizeBufNew = sizeStrBufOrigin + (2 + uLenStrNew + 1);
        shared_ptr<byte> pBufStrDataNew = shared_ptr<byte>(new byte[uSizeBufNew], default_delete<byte[]>());
        memset(pBufStrDataNew.get(), 0, uSizeBufNew);
        memcpy(pBufStrDataNew.get(), pStrings.get(), sizeStrBufOrigin); // 把之前老的复制到前面.
        memcpy(pBufStrDataNew.get() + sizeStrBufOrigin, &uLenStrNew, 1); // 写新添加字符串的长度
        memcpy(pBufStrDataNew.get() + sizeStrBufOrigin + 1, &uLenStrNew, 1); // 写新添加字符串的长度
        memcpy(pBufStrDataNew.get() + sizeStrBufOrigin + 2, newStr.c_str(), uLenStrNew + 1); // 把空结束符的字符串拷贝进去
        pStrings.swap(pBufStrDataNew);
        //
        uTotalAdd += (uSizeBufNew - sizeStrBufOrigin);
    } else {
        u16string newStr16 = toUtf16(newStr);
        uint16_t uSizeBufNew = 2 + (uLenStrNew + 1)*2;
        shared_ptr<byte> pBufStrDataNew = shared_ptr<byte>(new byte[uSizeBufNew], default_delete<byte[]>());
        memset(pBufStrDataNew.get(), 0, uSizeBufNew);
        memcpy(pBufStrDataNew.get(), pStrings.get(), sizeStrBufOrigin);
        memcpy(pBufStrDataNew.get() + sizeStrBufOrigin, &uLenStrNew, 2); // 写新添加字符串的长度.
        memcpy(pBufStrDataNew.get() + sizeStrBufOrigin + 2, newStr16.c_str(), (uLenStrNew+1)*2 ); // 
        pStrings.swap(pBufStrDataNew);
        //
        uTotalAdd += (uSizeBufNew - sizeStrBufOrigin);
    }
    
    // 字符串统计+1
    header.stringCount += 1; 
    // update meta data.
    header.stringsStart += sizeof(uint32_t);
    // 调码整 style起始位置, style偏移数组里面的值. 
    if (header.stylesStart > 0) {
        header.stylesStart += uTotalAdd;
    }
    
    // 整体字符串包大小也要改.
    header.header.size += uTotalAdd;
    //
    return uTotalAdd;
}

// return -1 measn failed. others means success.
uint32_t ResourcesParser::ResStringPool::getStrIdx(const std::string& destStr) {
    if (destStr.length() == 0) {
        return -1;
    }
    if (header.stringCount == 0) {
        return -1;
    }
    for (uint32_t idx = 0; idx<header.stringCount; ++idx) {
        uint32_t offset = *(pOffsets.get() + idx);
        char* str = ((char*)(pStrings.get() + offset + 2));
        std::string itemStr = (header.flags & ResStringPool_header::UTF8_FLAG) ? str : toUtf8((char16_t*)str);
        if (itemStr.compare(destStr) == 0) {
            return idx;
        }
    }
    return -1; //没有匹配上.
}

uint32_t ResourcesParser::EntryPool::addNewEntry(uint16_t flags, uint32_t idxResKeyName, uint8_t dataType, uint32_t idxValue) {
    uint32_t newEntryOffset = dataSize;
    // update pOffsets
    std::shared_ptr<uint32_t> pOffsetsNew = shared_ptr<uint32_t>(
            new uint32_t[offsetCount+1],
            default_delete<uint32_t[]>()
    );
    memcpy(pOffsetsNew.get(), pOffsets.get(), sizeof(uint32_t)*offsetCount);
    memcpy(pOffsetsNew.get()+offsetCount, &newEntryOffset, sizeof(uint32_t));
    pOffsets.swap(pOffsetsNew);
    // update offsetCount
    offsetCount += 1;

    // update pData.
    uint32_t newDataSize = dataSize + sizeof(ResTable_entry) + sizeof(Res_value);
    std::shared_ptr<byte> pDataNew = shared_ptr<byte>(
            new byte[newDataSize], 
            default_delete<byte[]>()
    );
    memcpy(pDataNew.get(), pData.get(), dataSize);
    ResTable_entry* pResTableEntry = (ResTable_entry*)(pDataNew.get()+dataSize);
    pResTableEntry->size = 8;
    pResTableEntry->flags = flags;
    pResTableEntry->key.index = idxResKeyName;
    Res_value* pResValue = (Res_value*)(pDataNew.get() + dataSize + sizeof(ResTable_entry));
    pResValue->size = 8;
    pResValue->res0 = 0;
    pResValue->dataType = dataType;
    pResValue->data = idxValue;
    pData.swap(pDataNew);
    //
    dataSize = newDataSize;

    uint32_t uAddSize = sizeof(uint32_t) + sizeof(ResTable_entry) + sizeof(Res_value);
    return uAddSize;
}

uint32_t ResourcesParser::ResTableType::addNewEntry(uint16_t flags, uint32_t idxResKeyName, uint8_t dataType, uint32_t idxValue) {
    uint32_t uAddSizeEntryPool = entryPool.addNewEntry(flags, idxResKeyName, dataType, idxValue);
    // update size and other info.
    header.header.size += uAddSizeEntryPool;
    header.entryCount += 1;
    header.entriesStart += sizeof(uint32_t); //多了一个偏移

    //
    uint32_t lastEntryOffset = *(entryPool.pOffsets.get() + (header.entryCount - 1));
    ResTable_entry* pResTableEntry = ((ResTable_entry*)(entryPool.pData.get() + lastEntryOffset));
    Res_value* pResValue = (Res_value*)((byte*)pResTableEntry + pResTableEntry->size);

    entries.push_back(pResTableEntry);
    values.push_back(pResValue);

    return uAddSizeEntryPool;
}

uint32_t ResourcesParser::addResKeyStr(std::string pkgName, std::string resType, std::string resKeyStr) {
    PackageResource* pPkgRes = nullptr; 
    if (pkgName.length() == 0) {
        std::map<std::string, PackageResourcePtr>::iterator it; 
        for (it = mResourceForPackageName.begin(); it!=mResourceForPackageName.end(); ++it) {
            cout<<"res_pkg_name: "<<it->first<<endl;
            pPkgRes = it->second.get();
            break; //只取第一个
        }
    } else {
        std::map<std::string, PackageResourcePtr>::iterator dest;
        dest = mResourceForPackageName.find(pkgName);
        if (dest!=mResourceForPackageName.end()) {
            pPkgRes = dest->second.get();
        }
    }
    if (pPkgRes == nullptr) {
        cout<< "[error] pPkgRes == nullptr"<<endl;
        return 0;
    }

    // add key string
    uint32_t uKeyAddSize = pPkgRes->pKeys.get()->addNewString(resKeyStr);
    pPkgRes->header.header.size += uKeyAddSize;
    mResourcesInfo.header.size += uKeyAddSize;
    // calc resKeyStr Id 
    if (pPkgRes->pKeys.get()->header.stringCount <= 0) {
        cout<<"[error] stringCount <= 0" <<endl;
        return 0;
    }
    uint32_t newResNameIdx = pPkgRes->pKeys.get()->header.stringCount - 1;

    // add value string to mGlobalStringPool
    std::string destValue = ("res/" + resType + "/" + resKeyStr + ".xml");
    uint32_t uValueAddSize = mGlobalStringPool.get()->addNewString(destValue);
    mResourcesInfo.header.size += uValueAddSize;
    // calc value string Id
    uint32_t newDestValueIdx = mGlobalStringPool.get()->header.stringCount - 1;

    // find the ResTableTypeId
    uint32_t idType = pPkgRes->pTypes.get()->getStrIdx(resType) + 1;
    if (idType < 0) {
        cout<<"[error] idType < 0" <<endl;
        return 0;
    }
    std::vector<ResTableTypePtr> vecResTableTypePtr =  pPkgRes->resTablePtrs[idType]; 
    if (vecResTableTypePtr.size() == 0) {
        cout<<"[error]["<<idType<<"] vecResTableTypePtr.size() == 0" << endl;
        return 0;
    }
    ResTableType* pResTableType = vecResTableTypePtr[0].get();

    // add new res entry info to res table.
    uint32_t uAddSizeNewEntry = pResTableType->addNewEntry(0x00, newResNameIdx, Res_value::TYPE_STRING, newDestValueIdx);
    pPkgRes->header.header.size += uAddSizeNewEntry;
    mResourcesInfo.header.size += uAddSizeNewEntry;
     
    //
    uint32_t newEntryIdx = pResTableType->entries.size() - 1;
    uint32_t newResId = 0x7f000000 | ((0x000000FF&idType)<<16) | (0x0000FFFF&newEntryIdx);
    return newResId;
}

bool ResourcesParser::saveToFile(const std::string& destFName) {
    FILE* pFile = fopen(destFName.c_str(), "w+");
    if (pFile == nullptr) {
       cout<<"pFile == nullptr, exit"<<endl;
       return false;
    }

    // 写 ResTable_header
    fwrite(&(mResourcesInfo), sizeof(ResTable_header), 1, pFile);

    // 写 Global String Pool
    writeStringPool(pFile, mGlobalStringPool.get());

    // 写 Table Package
    for (auto &item : mResourceForPackageName) {
        writePackageResource(pFile, item.second.get());
    }


    fclose( pFile );
    return true;
}

void ResourcesParser::writeStringPool(FILE* pFile, ResStringPool* pStringPool) {
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


void ResourcesParser::writePackageResource(FILE* pFile, PackageResource* pPkgRes) {
    if (pPkgRes == nullptr) {
        return;
    }
    // write ResTable_package
    fwrite(&(pPkgRes->header), sizeof(ResTable_package), 1, pFile);

    // write ResType string pool
    writeStringPool(pFile, pPkgRes->pTypes.get());

    // write ResName string pool
    writeStringPool(pFile, pPkgRes->pKeys.get());

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

void ResourcesParser::writeResTableType(FILE* pFile, ResTableType* pResTable) {
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

void ResourcesParser::writeResTableUnknown(FILE* pFile, ResTableTypeUnknown* pResTableUnknown) {
    ResChunk_header* pChunkHeader = (ResChunk_header*)pResTableUnknown->pChunkAllData.get();

    fwrite(pResTableUnknown->pChunkAllData.get(), 1, pChunkHeader->size, pFile);

}




