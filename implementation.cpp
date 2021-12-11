#include "interface.h"
#include "queue.h"
#include <vector>
#include <iostream>

struct MyClient : Client
{
    int arriveMinute; ///< The time at which MyClient arrives
	int banana; ///< Amount of bananas the MyClient wants to take
	int schweppes; ///< Amount of schweppes the MyClient wants to take
	int maxWaitTime; ///< The max amount of time the MyClient will wait before he departs
    int index;
	bool hasReservedB = false;
	bool hasReservedS = false;
	bool isWaiting = false;

	MyClient(int arrMin, int ban, int sch, int maxWait, int in = 0) : arriveMinute(arrMin), banana(ban), schweppes(sch), maxWaitTime(maxWait)
	{
		index = in;
	}

	MyClient(){}
};

struct MyStore : Store 
{
    bool isConsole = false;
    MyStore(bool _isConsole = false) {isConsole = _isConsole;}
    int indice = 0;
    int workers = 0, bananas = 0, schweppes = 0, MyClientsCount = 0;

    Queue<int> arrivalTimeB;
    Queue<int> arrivalTimeS;

    int reservedB = 0, reservedS = 0, sentForB = 0, sentForS = 0;

    std::vector<MyClient> cs;

	ActionHandler *actionHandler = nullptr;

	void setActionHandler(ActionHandler *handler) override 
    {
		actionHandler = handler;
	}

	void init(int workerCount, int startBanana, int startSchweppes) override
    {
        workers = workerCount;
        bananas = startBanana;
        schweppes = startSchweppes;
	}

    int addStock(ResourceType t)
    {
        if(t == ResourceType::banana)
        {
            if(isConsole)
                std::cout<<"D " << arrivalTimeB.front() << " banana\n";

            bananas += 100;
            sentForB--;
            return arrivalTimeB.pop();
        }
        
        if(isConsole)
            std::cout<<"D " << arrivalTimeS.front() << " schweppes\n";

        schweppes += 100;
        sentForS--;
        return arrivalTimeS.pop();;
    }

    bool checkForDeliveries(const int &minute)
    {
        bool hasDelivery = false;
        while(!arrivalTimeB.empty() && arrivalTimeB.front() <= minute)
        {
            if(!isConsole)
                actionHandler->onWorkerBack(arrivalTimeB.front(), ResourceType::banana);

            addStock(ResourceType::banana);
            hasDelivery = true;
        }
        
        while(!arrivalTimeS.empty() && arrivalTimeS.front() <= minute)
        {
            if(!isConsole)
                actionHandler->onWorkerBack(arrivalTimeS.front(), ResourceType::schweppes);

            addStock(ResourceType::schweppes);
            hasDelivery = true;
        }
        return hasDelivery;
    }

	void addClients(const Client *clients, int count) override 
    {
        MyClientsCount = count;
		for (int i = 0; i < count; i++)
        {
            MyClient c{clients[i].arriveMinute, clients[i].banana, clients[i].schweppes, clients[i].maxWaitTime, indice++};
            cs.push_back(c);
        }
	}

    bool checkForEnoughStock(int &bananas_, int &schweppes_)
    {
        bool hasB = false, hasS = false;
        if(bananas_ <= bananas)
            hasB = true;

        if(schweppes_ <= schweppes)
            hasS = true;

        return hasS && hasB;
    }

    void removeClient(MyClient &c)
    {
        for (size_t i = 0; i < cs.size(); i++)
            if (cs[i].index == c.index)
            {
                cs.erase(cs.begin() + i);
                break;
            }
    }

    void tryOrderSchweppes(MyClient &c, int &minute)
    {
        while(c.schweppes > schweppes + (sentForS * 100 - reservedS) && workers - (sentForS + sentForB) > 0 && !c.isWaiting && !c.hasReservedS)
        {
            sentForS++;
            c.hasReservedS = true;
            reservedS += c.schweppes;
            arrivalTimeS.enqueue(c.arriveMinute + 60);
            if(isConsole)
                std::cout<<"W " << c.arriveMinute << " schweppes\n";
            else
                actionHandler->onWorkerSend(c.arriveMinute, ResourceType::schweppes);
        }
        if (c.schweppes > schweppes && (workers - (sentForS + sentForB) > 0) && !c.hasReservedS)
        {
            reservedS += c.schweppes;
            c.hasReservedS = true;
        }
    }

    void tryOrderBanana(MyClient &c, int minute)
    {
        while(c.banana > bananas + (sentForB * 100 - reservedB) && workers - (sentForS + sentForB) > 0 && !c.isWaiting && !c.hasReservedB)
        {
            sentForB++;
            reservedB += c.banana;
            c.hasReservedB = true;
            arrivalTimeB.enqueue(c.arriveMinute + 60);
            if(isConsole)
                std::cout<<"W " << c.arriveMinute << " banana\n";
            else
                actionHandler->onWorkerSend(c.arriveMinute, ResourceType::banana);
        }

        if (c.banana > bananas && workers - (sentForS + sentForB) > 0 && !c.hasReservedB)
        {
            reservedB += c.banana;
            c.hasReservedB = true;
        }
    }

    int checkWaitingClients(MyClient &c, const size_t &MyClientDepMin, int &currMin)
    {
        int waitingMyClientDepMin = c.maxWaitTime + c.arriveMinute;
        if(waitingMyClientDepMin <= MyClientDepMin && waitingMyClientDepMin <= currMin)
        {
            while(!arrivalTimeB.empty() && arrivalTimeB.front() <= waitingMyClientDepMin)//if there was banana delivery before he goes
            {
                int minAdded = addStock(ResourceType::banana);
                if(checkForEnoughStock(c.banana, c.schweppes))
                    return minAdded;
            }

            while(!arrivalTimeS.empty() && arrivalTimeS.front() <= waitingMyClientDepMin)// if there was schweppes delivery before he goes
            {
                int minAdded = addStock(ResourceType::schweppes);
                if(checkForEnoughStock(c.banana, c.schweppes))
                    return minAdded;
            }
            return waitingMyClientDepMin; //there were no deliveries he departs with all he can get
        }
        else return -1;
    }

    void MyClientDeparture(MyClient &c, int min)
    {   
        if(c.hasReservedB)
            reservedB -= c.banana;

        if(c.hasReservedS)
            reservedS -= c.schweppes;
            
        if (c.banana >= bananas)
        {
            c.banana = bananas;
            bananas = 0;
        }

        else
            bananas -= c.banana;

        if (c.schweppes >= schweppes)
        {
            c.schweppes = schweppes;
            schweppes = 0;
        }

        else
            schweppes -= c.schweppes;

        if(isConsole)
                std::cout<<c.index<<" "<< min << " " << c.banana << " " << c.schweppes << std::endl;
            else
                actionHandler->onClientDepart(c.index, min, c.banana, c.schweppes);
        removeClient(c);
        MyClientsCount--;
        
    }

	void advanceTo(int minute) override  
    {
		for (int i = 0; i < MyClientsCount && !cs.empty(); i++)
        {   
            int MyClientDepTime = cs[i].arriveMinute + cs[i].maxWaitTime;
            for (int j = 0; j < i; j++) //going threw the waiting MyClients
            {
                if(checkWaitingClients(cs[j], MyClientDepTime, minute) != -1)
                {
                    MyClientDeparture(cs[j], MyClientDepTime);
                    --i;
                    --j;
                }
            }

            if(cs[i].arriveMinute <= minute) //check if the MyClient has arrived until the given min
            {
                checkForDeliveries(cs[i].arriveMinute); //check for deliveries before the arrival

                if ((cs[i].isWaiting && MyClientDepTime <= minute) || checkForEnoughStock(cs[i].banana, cs[i].schweppes))
                {
                    MyClientDeparture(cs[i], cs[i].arriveMinute);
                    i--;
                    continue;
                }

                if(workers - (sentForB + sentForS) == 1 && cs[i].banana != 0 && cs[i].schweppes != 0 && bananas < cs[i].banana && schweppes < cs[i].schweppes)
                {
                    if(cs[i].schweppes - schweppes <= cs[i].banana - bananas)
                        tryOrderBanana(cs[i], minute);
                            else tryOrderSchweppes(cs[i], minute);
                }
   
                tryOrderBanana(cs[i], minute);
                tryOrderSchweppes(cs[i], minute);
                cs[i].isWaiting = true;

                if (MyClientDepTime <= minute)
                {
                    MyClient curr = cs[i];
                    for (size_t j = i + 1; j < MyClientsCount; j++)
                    {
                        if(checkWaitingClients(cs[j], MyClientDepTime, minute) != -1)
                        {
                            MyClientDeparture(cs[j], MyClientDepTime);
                            --i;
                            --j;
                        }
                    }
                    //int enoughStockMin = checkWaitingClients(cs[i], MyClientDepTime, minute);
                    //if(enoughStockMin != -1)
                    //    MyClientDeparture(curr, enoughStockMin);
                    //else MyClientDeparture(curr, MyClientDepTime);
                    
                    for (size_t j = i + 1; j < MyClientsCount; j++)
                    {
                        if(cs[j].arriveMinute < MyClientDepTime)
                        {
                            if(checkForEnoughStock(cs[j].banana, cs[j].schweppes))
                                MyClientDeparture(cs[j], cs[j].arriveMinute);
                            else if(!cs[j].isWaiting)
                            {
                                tryOrderBanana(cs[j],minute);
                                tryOrderSchweppes(cs[j], minute);
                            }
                        }
                    }
                    bool hasDeparted = false;

                    while(!arrivalTimeB.empty() && arrivalTimeB.front() <= MyClientDepTime)
                    {
                        int minuteAdded = addStock(ResourceType::banana);
                        if(!isConsole)
                            actionHandler->onWorkerBack(minuteAdded, ResourceType::banana);
                        if(checkForEnoughStock(curr.banana, curr.schweppes))
                        {
                            MyClientDeparture(curr, minuteAdded);
                            hasDeparted = true;
                            break;
                        }
                    }

                    while(!arrivalTimeS.empty() && arrivalTimeS.front() < MyClientDepTime)
                    {
                        int minuteAdded = addStock(ResourceType::schweppes);
                        if(!isConsole)
                            actionHandler->onWorkerBack(minuteAdded, ResourceType::schweppes);
                        if(checkForEnoughStock(curr.banana, curr.schweppes))
                        {
                            MyClientDeparture(curr, minuteAdded);
                            hasDeparted = true;
                            break;
                        }
                    }
                    
                    if(!hasDeparted)
                        MyClientDeparture(curr, MyClientDepTime);

                    i--;
                    continue;
                }

                cs[i].isWaiting = true;
            }
        }
	}

	int getBanana() const 
    {
		return bananas;
	}

	int getSchweppes() const 
    {
		return schweppes;
	}
};

Store *createStore() 
{
	return new MyStore();
}