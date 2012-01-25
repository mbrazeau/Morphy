#include "NexusParse.h"

CNexusParse::CNexusParse()
{
}

CNexusParse::~CNexusParse()
{
    if (m_cTaxa)
    {
        delete m_cTaxa;
    }
    if (m_cChars)
    {
        delete m_cChars;
    }
    if (m_cTrees)
    {
        delete m_cTrees;
    }
    if (m_cData)
    {
        delete m_cData;
    }
    if (m_cNexusReader)
    {
        delete m_cNexusReader;
    }
}

bool CNexusParse::ReadNexusFile(string *infname, string *outfname)
{
    bool bRet = false;

    /*
     * if a new fails, the the destructor will take care of freeing
     * any memory that happened to work...
     */
    m_cNexusReader = new CNexusReader(infname, outfname);
    m_cTaxa  = new NxsTaxaBlock();
    m_cChars = new NxsCharactersBlock(0, 0);
    m_cData  = new NxsDataBlock(0, 0);
    if (m_cNexusReader && m_cTaxa && m_cChars && m_cData)
    {
        m_cTrees = new NxsTreesBlock(m_cTaxa);
        if (m_cTrees)
        {
            m_cNexusReader->Add(m_cTaxa);
            m_cNexusReader->Add(m_cChars);
            m_cNexusReader->Add(m_cTrees);
            m_cNexusReader->Add(m_cData);
           
            istream &iStream = m_cNexusReader->GetInStream();
            if (iStream)
            {
                CNexusToken token(iStream, m_cNexusReader->GetOutStream());
                m_cNexusReader->Execute(token);
                bRet = true;
            }
        }
    }
    return bRet;
}

void CNexusParse::Report()
{
    ostream &oStream = m_cNexusReader->GetOutStream();
    m_cTaxa->Report (oStream);
    m_cChars->Report(oStream);
    m_cTrees->Report(oStream);
    m_cData->Report (oStream);
}
