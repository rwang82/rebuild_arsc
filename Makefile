rp : \
	main.cpp \
	ResourcesParser/ResourcesParserInterpreter.h \
	ResourcesParser/ResourcesParserInterpreter.cpp \
	ResourcesParser/ResourcesParser.h \
	ResourcesParser/ResourcesParser.cpp \
	ResourcesParser/ResourceTypes.h \
	ResourcesParser/ResourceTypes.cpp \
	ResourcesParser/configuration.h \
	ResourcesParser/ByteOrder.h
	g++ main.cpp ResourcesParser/ResourcesParserInterpreter.cpp ResourcesParser/ResourcesParser.cpp ResourcesParser/ResourceTypes.cpp -std=c++11 -o rp

.PHONY : clean
clean :
	rm rp

