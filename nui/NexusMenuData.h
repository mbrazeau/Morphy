#include "NexusMenuBase.h"

class CNexusMenuData
{
public:
    CNexusMenuData(string strMenuTitle);
    ~CNexusMenuData();
    void AddMenuItem(CNexusMenuBase* pMenuItem);
    bool RunSelection(string strInput, CNexusUserInterface *pNexusUserInterface);
    bool Help(bool bForceShowMenu);
    string GetPrompt();

protected:

    std::string &ltrim(std::string &s);
    std::string &rtrim(std::string &s);
    std::string &trim(std::string &s);
    void SplitInput(string strInput, string *command, string *value);
    void PrintError(string strInput, vector<CNexusMenuBase*> pMenuItems);
    vector<CNexusMenuBase*> GetMenuSelection(string strInput);

    bool bMenuShown;
    string m_strMenuTitle;
    vector <CNexusMenuBase*> m_vMenu;
};
