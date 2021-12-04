#include "interface.h"
#include "queue.h"
#include <vector>
#include <iostream>

struct MyStore : Store 
{
    int indice = 0;
    int workers = 0, bananas = 0, schweppes = 0, clientsCount = 0;

    Queue<int> arrivalTimeB;
    Queue<int> arrivalTimeS;

    int reservedB = 0, reservedS = 0, sentForB = 0, sentForS = 0;

    std::vector<Client> cs;

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
            //std::cout<<"D " << arrivalTimeB.front() << " banana\n";
            bananas += 100;
            sentForB--;
            return arrivalTimeB.pop();
        }
        
        //std::cout<<"D " << arrivalTimeS.front() << " schweppes\n";
        schweppes += 100;
        sentForS--;
        return arrivalTimeS.pop();;
    }

    bool checkForDeliveries(const int &minute)
    {
        bool hasDelivery = false;
        while(!arrivalTimeB.empty() && arrivalTimeB.front() <= minute)
        {
            actionHandler->onWorkerBack(arrivalTimeB.front(), ResourceType::banana);
            addStock(ResourceType::banana);
            hasDelivery = true;
        }
        
        while(!arrivalTimeS.empty() && arrivalTimeS.front() <= minute)
        {
            actionHandler->onWorkerBack(arrivalTimeS.front(), ResourceType::schweppes);
            addStock(ResourceType::schweppes);
            hasDelivery = true;
        }
        return hasDelivery;
    }

	void addClients(const Client *clients, int count) override 
    {
        clientsCount = count;
		for (int i = 0; i < count; i++)
        {
            Client c{clients[i].arriveMinute, clients[i].banana, clients[i].schweppes, clients[i].maxWaitTime, indice++};
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

    void removeClient(Client &c)
    {
        for (size_t i = 0; i < cs.size(); i++)
            if (cs[i].index == c.index)
            {
                cs.erase(cs.begin() + i);
                break;
            }
    }

    void tryOrderSchweppes(Client &c, int &minute)
    {
        while(c.schweppes > schweppes + (sentForS * 100 - reservedS) && workers - (sentForS + sentForB) > 0 && !c.isWaiting && !c.hasReservedS)
        {
            sentForS++;
            c.hasReservedS = true;
            reservedS += c.schweppes;
            arrivalTimeS.enqueue(c.arriveMinute + 60);
            actionHandler->onWorkerSend(c.arriveMinute, ResourceType::schweppes);
        }
        if (c.schweppes > schweppes && (workers - (sentForS + sentForB) > 0) && !c.hasReservedS)
        {
            reservedS += c.schweppes;
            c.hasReservedS = true;
        }
    }

    void tryOrderBanana(Client &c, int minute)
    {
        while(c.banana > bananas + (sentForB * 100 - reservedB) && workers - (sentForS + sentForB) > 0 && !c.isWaiting && !c.hasReservedB)
        {
            sentForB++;
            reservedB += c.banana;
            c.hasReservedB = true;
            arrivalTimeB.enqueue(c.arriveMinute + 60);
            actionHandler->onWorkerSend(c.arriveMinute, ResourceType::banana);
        }

        if (c.banana > bananas && workers - (sentForS + sentForB) > 0 && !c.hasReservedB)
        {
            reservedB += c.banana;
            c.hasReservedB = true;
        }
    }

    bool checkWaitingClients(Client &c, const size_t &clientDepMin, int &currMin)
    {
        int waitingClientDepMin = c.maxWaitTime + c.arriveMinute;
        if(waitingClientDepMin <= clientDepMin && waitingClientDepMin <= currMin)
        {
            while(!arrivalTimeB.empty() && arrivalTimeB.front() <= waitingClientDepMin)//if there was banana delivery before he goes
            {
                int minAdded = addStock(ResourceType::banana);
                if(checkForEnoughStock(c.banana, c.schweppes))
                    return true;
            }

            while(!arrivalTimeS.empty() && arrivalTimeS.front() <= waitingClientDepMin)// if there was schweppes delivery before he goes
            {
                int minAdded = addStock(ResourceType::schweppes);
                if(checkForEnoughStock(c.banana, c.schweppes))
                    return true;
            }
            return true; //there were no deliveries he departs with all he can get
        }
        else return false;
    }

    void clientDeparture(Client &c, int min)
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

        actionHandler->onClientDepart(c.index, min, c.banana, c.schweppes);
        removeClient(c);
        clientsCount--;
        //std::cout<<c.index<<" "<< min << " " << c.banana << " " << c.schweppes << std::endl;
    }

	void advanceTo(int minute) override  
    {
		for (int i = 0; i < clientsCount && !cs.empty(); i++)
        {   
            int clientDepTime = cs[i].arriveMinute + cs[i].maxWaitTime;
            for (int j = 0; j < i; j++) //going threw the waiting clients
            {
                if(checkWaitingClients(cs[j], clientDepTime, minute))
                {
                    clientDeparture(cs[j], clientDepTime);
                    --i;
                    --j;
                }
            }

            if(cs[i].arriveMinute <= minute) //check if the client has arrived until the given min
            {
                checkForDeliveries(cs[i].arriveMinute); //check for deliveries before the arrival

                if ((cs[i].isWaiting && clientDepTime <= minute) || checkForEnoughStock(cs[i].banana, cs[i].schweppes))
                {
                    clientDeparture(cs[i], cs[i].arriveMinute);
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

                if (clientDepTime <= minute)
                {
                    Client curr = cs[i];
                    for (size_t j = i + 1; j < clientsCount; j++)
                    {
                        if(checkWaitingClients(cs[j], clientDepTime, minute))
                        {
                            clientDeparture(cs[j], clientDepTime);
                            --i;
                            --j;
                        }
                    }
                    clientDeparture(curr, clientDepTime);
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