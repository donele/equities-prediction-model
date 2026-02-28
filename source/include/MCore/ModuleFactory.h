#ifndef __MCore_ModuleFactory__
#define __MCore_ModuleFactory__
#include <MFramework/MModuleBase.h>

class ModuleFactory {
public:
	ModuleFactory(){}
	~ModuleFactory(){}
	MModuleBase* getModule(const std::string& className,
			const std::string& moduleName, const std::multimap<std::string, std::string>& uniqueName);
private:
};

#endif
