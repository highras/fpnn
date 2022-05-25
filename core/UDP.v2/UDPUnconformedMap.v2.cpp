#include "UDPAssembler.v2.h"	//-- For UDPPAckage definition.
#include "UDPUnconformedMap.v2.h"

using namespace fpnn;

//==============================================//
//--              Resend Tracer               --//
//==============================================//
void ResendTracer::update(uint32_t currentUna, size_t limitedCount)
{
	if (count != 0)
	{
		count = (uint32_t)limitedCount;

		uint32_t diff = currentUna - una;

		una = currentUna;

		if (step <= diff)
		{
			lastSeq = una;
			step = 0;
		}
		else
		{
			step -= diff;

			if (step >= count)
				step = 0;

			lastSeq = una + step;
		}
	}
	else
	{
		count = (uint32_t)limitedCount;
		una = currentUna;
		lastSeq = currentUna;
		step = 0;
	}
}

void ResendTracer::reset()
{
	lastSeq = una;
	step = 0;
}

//==============================================//
//--         UDP Unconformed Map              --//
//==============================================//
UDPUnconformedMap::~UDPUnconformedMap()
{
	for (auto node: _sentQueue)
	{
		if (node->package)
			delete node->package;

		delete node;
	}
}

void UDPUnconformedMap::insert(uint32_t seqNum, UDPPackage* package)
{
	PackageNode* node = new PackageNode(seqNum, package);
	_unconformedMap[seqNum] = node;
	_sentQueue.push_back(node);
}

void UDPUnconformedMap::fetchResendPackages(int freeSpace, int64_t threshold, std::list<UDPUnconformedMap::PackageNode*>& canbeAssembledPackages)
{
	const int assembledSectionExtraBytes = ARQConstant::AssembledPackageLengthFieldSize - 1;	//-- 1: version field size.
	const int mimimumSpaceRequire = assembledSectionExtraBytes + ARQConstant::PackageMimimumLength;

	if (freeSpace < ARQConstant::PackageMimimumLength + assembledSectionExtraBytes)
		return;

	int64_t now = slack_real_msec();
	for (auto it = _sentQueue.begin(); it != _sentQueue.end(); )
	{
		PackageNode* node = *it;

		if (node->package != NULL)
		{
			if (node->package->lastSentMsec <= threshold)
			{
				//-- There is not 'requireDeleted == true' package, and no 'resending == true' package.
				if (_enableExpireCheck)
					node->package->expireCheck(now);
				
				if (freeSpace >= assembledSectionExtraBytes + (int)(node->package->len))
				{
					canbeAssembledPackages.push_back(node);
					it = _sentQueue.erase(it);

					freeSpace -= (assembledSectionExtraBytes + (int)(node->package->len));
					if (freeSpace < mimimumSpaceRequire)
						break;
				}
				else
					it++;
			}
			else
				break;
		}
		else
		{
			delete node;
			it = _sentQueue.erase(it);
		}
	}

	for (auto node: canbeAssembledPackages)
		_sentQueue.push_back(node);
}

void UDPUnconformedMap::assemblePackages(UDPPackage* package,
	std::list<UDPUnconformedMap::PackageNode*>& canbeAssembledPackages, CurrentSendingBuffer* sendingBuffer)
{
	sendingBuffer->reset();
	sendingBuffer->changeForPackageAssembling();

	if (package)
		sendingBuffer->assemblePackage(package);

	for (auto node: canbeAssembledPackages)
		sendingBuffer->assemblePackage(node->package);
}

void UDPUnconformedMap::assemblePackages(std::set<UDPUnconformedMap::PackageNode*>& selectedPackages,
	std::list<UDPUnconformedMap::PackageNode*>& supplementaryPackages, CurrentSendingBuffer* sendingBuffer)
{
	sendingBuffer->reset();
	sendingBuffer->changeForPackageAssembling();

	for (auto node: selectedPackages)
		sendingBuffer->assemblePackage(node->package);

	for (auto node: supplementaryPackages)
		sendingBuffer->assemblePackage(node->package);
}

bool UDPUnconformedMap::prepareSendingBuffer(int MTU, int64_t threshold, UDPPackage* package, CurrentSendingBuffer* sendingBuffer)
{
	int freeSpace = MTU - ARQConstant::AssembledPackageHeaderSize - (int)(package->len);

	std::list<UDPUnconformedMap::PackageNode*> canbeAssembledPackages;
	fetchResendPackages(freeSpace, threshold, canbeAssembledPackages);
	if (canbeAssembledPackages.empty())
		return false;

	assemblePackages(package, canbeAssembledPackages, sendingBuffer);
	return true;
}

bool UDPUnconformedMap::prepareSendingBuffer(int MTU, int64_t threshold, CurrentSendingBuffer* sendingBuffer)
{
	int freeSpace = MTU - ARQConstant::AssembledPackageHeaderSize;

	std::list<UDPUnconformedMap::PackageNode*> canbeAssembledPackages;
	fetchResendPackages(freeSpace, threshold, canbeAssembledPackages);
	if (canbeAssembledPackages.empty())
		return false;

	assemblePackages(NULL, canbeAssembledPackages, sendingBuffer);
	return true;
}

void UDPUnconformedMap::cleanByUNA(uint32_t una, int64_t now, int &count, int64_t &totalDelay)
{
	count = 0;
	totalDelay = 0;

	for (auto it = _unconformedMap.begin(); it != _unconformedMap.end(); )
	{
		uint32_t a = una - it->first;
		uint32_t b = it->first - una;

		if (a <= b)
		{
			totalDelay += now - it->second->package->firstSentMsec;
			count += 1;

			if (it->second->package->resending == false)
				delete it->second->package;
			else
				it->second->package->requireDeleted = true;

			it->second->package = NULL;
			it = _unconformedMap.erase(it);
		}
		else
			it++;
	}
}

void UDPUnconformedMap::cleanByAcks(const std::unordered_set<uint32_t>& acks, int64_t now, int &count, int64_t &totalDelay)
{
	count = 0;
	totalDelay = 0;

	for (auto ack: acks)
	{
		auto it = _unconformedMap.find(ack);
		if (it != _unconformedMap.end())
		{
			totalDelay += now - it->second->package->firstSentMsec;
			count += 1;

			if (it->second->package->resending == false)
				delete it->second->package;
			else
				it->second->package->requireDeleted = true;

			it->second->package = NULL;
			_unconformedMap.erase(it);
		}
	}
}

UDPPackage* UDPUnconformedMap::v1_fetchResentPackage_normalMode(int64_t threshold, uint32_t& seqNum)
{
	PackageNode* target = NULL;
	for (auto it = _sentQueue.begin(); it != _sentQueue.end(); )
	{
		PackageNode* node = *it;

		if (node->package != NULL)
		{
			if (node->package->lastSentMsec <= threshold)
			{
				//-- There is not 'requireDeleted == true' package, and no 'resending == true' package.

				target = node;
				it = _sentQueue.erase(it);
				break;
			}
			else
				break;
		}
		else
		{
			delete node;
			it = _sentQueue.erase(it);
		}
	}

	if (target)
	{
		_sentQueue.push_back(target);

		seqNum = target->seqNum;
		return target->package;
	}

	return NULL;
}
