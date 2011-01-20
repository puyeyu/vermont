/*
 * VERMONT
 * Copyright (C) 2010 Tobias Limmer <limmer@cs.fau.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#if !defined(PRIORITYSYSTEM_H__)
#define PRIORITYSYSTEM_H__


#include "BasePacketSelector.hpp"
#include "IpPacketSelector.hpp"

#include <map>
#include <vector>

using namespace std;


struct HostData
{
	double priority;
	double oldpriority; // priority before monitoring was started
	double weight;

	uint32_t ip;
	int16_t assignedIdsId; /**< -1 if not assigned */
	uint64_t nextTraffic; // estimation!
	uint64_t lastTraffic;
	uint32_t id; /**< sorted id for this queue, used for fast dropping */

	struct timeval startMon; /**< time when host was assigned for monitoring */

	HostData(uint32_t ip, double p, double w);
	bool belowMinMonTime(struct timeval& tv, struct timeval& minmontime);
};


/**
 * included in Packets sent to PCAPExporterMem and IpPacketSelector
 * is also manipulated by that class
 * one instance per queue is created
 */
struct PacketHostInfo
{
	HostData* currentHost;
	list<HostData*> sortedHosts;
	list<HostData*> removedHosts;
	HostHashtable* selectorData;
	uint32_t maxHostId;
	uint32_t hostCount;
	uint64_t controlDropped;
};


struct PriorityNetConfig
{
	uint32_t subnet;
	uint32_t mask;
	uint8_t maskbits;
	uint32_t hostcount;
	double weight;

	PriorityNetConfig(uint32_t subnet, uint32_t mask, uint8_t maskbits, double weight);
};

struct IDSData
{
	uint64_t maxOctets;		/**< maximum octets that can be monitored by IDS in one interval */
	bool slowStart;			/**< determines whether we are in first phase of IDS max. rate detection */
	list<HostData*> hosts;  /**< hosts assigned to ids */
	uint32_t hostcount;

	// control system related parameters
	double estRatio; /**< estimation ratio: predicted rate = estRatio * original predicted rate by summing traffic predictions */
	double kp; 		 /**< gain for p-controller */
	uint64_t expOctets; /**< traffic expected by summing up all hosts' traffic (taken from last round) */

	uint64_t lastForwOct;
	uint64_t curForwOct;
	uint64_t lastForwPkt;
	uint64_t curForwPkt;

	uint64_t curDropOct;
	uint64_t lastDropOct;
	uint64_t curDropPkt;
	uint64_t lastDropPkt;
	uint64_t controlDropOct;

	uint32_t maxQueueSize;
	uint32_t curQueueSize;

	IDSData(uint64_t maxoctets, double kp)
		: maxOctets(maxoctets),
		  slowStart(true),
		  hostcount(0),
		  estRatio(1.0),
		  kp(kp),
		  expOctets(0),
		  lastForwOct(0),
		  curForwOct(0),
		  lastForwPkt(0),
		  curForwPkt(0),
		  curDropOct(0),
		  lastDropOct(0),
		  curDropPkt(0),
		  lastDropPkt(0),
		  controlDropOct(0),
		  maxQueueSize(0),
		  curQueueSize(0)
	{}
};


class PriorityPacketSelector : public BasePacketSelector
{
public:
	PriorityPacketSelector(list<PriorityNetConfig>& pnc, double startprio, struct timeval minmontime);
	virtual ~PriorityPacketSelector();
	virtual int decide(Packet *p);
	virtual void updateData(struct timeval curtime, list<IDSLoadStatistics>& lstats);
	virtual void setQueueCount(uint32_t n);
	virtual void setUpdateInterval(uint32_t ms);
	virtual void start();
	virtual void stop();
	void queueUtilization(uint32_t queueid, uint32_t maxsize, uint32_t cursize);
	virtual void setFlowExporter(Destination<IpfixRecord*>* di);


private:

	struct IpfixData {
		uint32_t ip;
		uint8_t monitored;
		uint64_t octets;
		uint64_t flowStartTime;
		uint64_t flowEndTime;
	};

	static const uint32_t WARN_HOSTCOUNT;

	list<PriorityNetConfig> config;
	list<HostData*> hosts;
	uint32_t hostCount;
	map<uint32_t, HostData*> ip2host;
	double startPrio;
	IpPacketSelector ipSelector;
	uint32_t maxHostPrioChange;	/**< called 'm' in paper */
	double prioSum;
	struct timeval minMonTime; /**< minimal monitoring time in milliseconds */
	uint64_t discardOctets;
	struct timeval startTime;
	struct timeval roundStart;
	uint32_t updateInterval; /**< update interval in ms */
	PacketHostInfo** packetHostInfo;
	PacketHostInfo** newPacketHostInfo;

	list<HostData*> restHosts; /**< hosts that are currently not monitored */
	vector<IDSData> ids;
	uint64_t round;
	Destination<IpfixRecord*>* flowExporter;

	boost::shared_ptr<IpfixRecord::SourceID> sourceId;
	boost::shared_ptr<TemplateInfo> templateInfo;

	static InstanceManager<IpfixDataRecord> dataRecordIM;

	uint32_t insertSubnet(uint32_t subnet, uint8_t maskbits, double weight);
	void updateTrafficEstimation();
	void calcMaxHostPrioChange();
	void updatePriorities();
	void assignHosts2IDS();
	void setIpConfig();
	void updateIDSMaxRate();
	void updateEstRatio();
	bool wasHostDropped(HostData* host);


};


#endif