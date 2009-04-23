#include "P2PDetectorCfg.h"


P2PDetectorCfg* P2PDetectorCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new P2PDetectorCfg(e);
}

P2PDetectorCfg::P2PDetectorCfg(XMLElement* elem)
    : CfgHelper<P2PDetector, P2PDetectorCfg>(elem, "p2pDetector"),
      intLength(300000),
      subnet(0),
      subnetmask(0)
{
	string ip;
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("interval")) {
			intLength = getInt("interval");
		} else if (e->matches("subnet")) {
			ip = e->getFirstText();
		} else if (e->matches("analyzerid")) {
			analyzerid = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown P2PDetector config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	
	if (analyzerid=="") THROWEXCEPTION("P2PDetectorCfg: analyzerid not set in configuration!");
	if (!setSubnet(ip)) THROWEXCEPTION("P2PDetectorCfg: subnet is not a valid!");
	intLength *= 1000;
	
	idmefTemplate = "idmef/templates/p2pdetector_template.xml";
}

P2PDetectorCfg::~P2PDetectorCfg()
{
}

P2PDetector* P2PDetectorCfg::createInstance()
{
    instance = new P2PDetector(intLength, subnet, subnetmask, analyzerid, idmefTemplate);
    return instance;
}

bool P2PDetectorCfg::deriveFrom(P2PDetectorCfg* old)
{
    return false;
}

uint32_t P2PDetectorCfg::stringToIP(string& str){
	
}
/**
 * sets subnet and subnetmask if the string is a valid IP-address with subnetmask
 * in the CIDR format (e.g. 192.168.1.0/24)
 */
bool P2PDetectorCfg::setSubnet(string& str){
	int parts[4] = {0,0,0,0};
	int index = 0;
	int numDots=0;
	int numNum=0;
	int len = str.length();
	bool allowNum = true; //number character is allowed for the next position
	bool allowDot = false; //dot is allowed for the next position
	bool allowSlash = false; //slash character is allowed for the next position
					 
	for(int i=0; i < str.length(); i++){
		//number
		if((str[i] >= '0') && (str[i] <= '9')){
			if(!allowNum)
				return false;
		
			if(numNum > 0)
				parts[index] *= 10;
			parts[index] += str[i] - '0';
			numNum++;
			
			if(numNum == 3)
				allowNum = false;
			if(index < 3)
				allowDot = true;
			if(index == 3){
				allowDot = false;
				allowSlash = true;
				if(str.length()-i == 1)
					subnetmask = 32;
			}
		}
		//Dot
		else if(str[i] == '.'){
			if(!allowDot || (str.length()-i == 1))
				return false;

			index++;
			numDots++;
			numNum = 0;
			allowDot = false;
			allowNum = true;
		}
		//slash
		else if(str[i] == '/'){
			if(!allowSlash || (str.length()-i > 3) || (str.length()-i < 2))
				return false;
			
			if((str[i+1] < '0') || (str[i+1] > '9'))
				return false;

			subnetmask = str[i+1] - '0';
			
			if(str.length()-i == 3){
				if((str[i+2] < '0') || (str[i+2] > '9'))
					return false;
				subnetmask *= 10;
				subnetmask += str[i+2] - '0';
			}

			if(subnetmask > 32)
				return false;

			break;
		}
		//other character		
		else
			return false;

	}

	if((index != 3) || (parts[0] > 255) || (parts[1] > 255) || (parts[2] > 255) || (parts[3] > 255))
		return false;

	for(int i=0; i<=3; i++){
		((uint8_t*)(&subnet))[i] = (uint8_t)parts[i];
	}
	
	//convert subnetmask to CIDR format
	uint32_t tmpMask = 0;
	for(int i = subnetmask; i>0; i--)
		tmpMask = (tmpMask >> 1) + 0x80000000;
	for(int i = 0; i<=3; i++)
		((uint8_t*)&subnetmask)[3-i] = ((uint8_t*)&tmpMask)[i];
	

	return true;
}