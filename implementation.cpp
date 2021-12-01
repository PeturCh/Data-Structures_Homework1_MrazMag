#include "interface.h"
#include <iostream>
#include <vector>
#include <queue>

struct MyStore : Store 
{
    int indice = 0;
    int workers = 0, bananas = 0, schweppes = 0, clientsCount = 0;

    std::queue<int> arrivalTimeB;
    std::queue<int> arrivalTimeS;

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
            int arriveTime = arrivalTimeB.front();
            bananas += 100;
            sentForB--;
            arrivalTimeB.pop();
            return arriveTime; // u have to implement pop with return value;
        }
        
        //std::cout<<"D " << arrivalTimeS.front() << " schweppes\n";
        int arriveTime = arrivalTimeS.front();
        schweppes += 100;
        sentForS--;
        arrivalTimeS.pop();
        return arriveTime;
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
            arrivalTimeS.push(c.arriveMinute + 60);
            std::cout << "W " << c.arriveMinute << " schweppes : " << c.index << " " << minute<<std::endl;
            //actionHandler->onWorkerSend(c.arriveMinute, ResourceType::schweppes);
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
            std::cout<<"vlizaB\n";
            sentForB++;
            reservedB += c.banana;
            c.hasReservedB = true;
            arrivalTimeB.push(c.arriveMinute + 60);
            std::cout << "W " << c.arriveMinute << " banana : " << "index: " << c.index << " " << minute<<std::endl;
            //actionHandler->onWorkerSend(c.arriveMinute, ResourceType::banana);
        }

        if (c.banana > bananas && workers - (sentForS + sentForB) > 0 && !c.hasReservedB)
        {
            reservedB += c.banana;
            c.hasReservedB = true;
        }
    }

    void clientDeparture(Client &c, int min)
    {   
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

        if(c.hasReservedB)
            reservedB -= c.banana;

        if(c.hasReservedS)
            reservedS -= c.schweppes;

        //std::cout<<c.index<<" "<< min << " " << c.banana << " " << c.schweppes << std::endl;
    }

	void advanceTo(int minute) override  
    {
		for (int i = 0; i < clientsCount && !cs.empty(); i++)
        {   
            int clientDepTime = cs[i].arriveMinute + cs[i].maxWaitTime;
            for (int j = 0; j < i; j++) //going threw the waiting clients
            {
                int waitingClientDepMin = cs[j].arriveMinute + cs[j].maxWaitTime;
                if (waitingClientDepMin <= cs[i].arriveMinute && waitingClientDepMin <= minute) //if someone has to go before the arrival of the new client
                {
                    bool hasDeparted = false;
                    while(!arrivalTimeB.empty() && arrivalTimeB.front() <= waitingClientDepMin)//if there was banana delivery before he goes
                    {
                        int minAdded = addStock(ResourceType::banana);
                        if(checkForEnoughStock(cs[j].banana, cs[j].schweppes))
                        {
                            clientDeparture(cs[j], minAdded);
                            //actionHandler->onClientDepart(cs[j].index, minAdded, cs[j].banana, cs[j].schweppes);
                            std::cout<<"cd: "<<cs[j].index<<std::endl;
                            removeClient(cs[j]);
                            hasDeparted = true;
                            clientsCount--;
                            i--;
                            j--;
                        }
                    }
                    if (hasDeparted) //if he only needed bananas and they have arrived
                        continue;
                    
                    while(!arrivalTimeS.empty() && arrivalTimeS.front() <= waitingClientDepMin)// if there was schweppes delivery before he goes
                    {
                        int minAdded = addStock(ResourceType::schweppes);
                        if(checkForEnoughStock(cs[j].banana, cs[j].schweppes))
                        {
                            clientDeparture(cs[j], minAdded);
                            //actionHandler->onClientDepart(cs[j].index, minAdded, cs[j].banana, cs[j].schweppes);
                            std::cout<<"cd: "<<cs[j].index<<std::endl;
                            removeClient(cs[j]);
                            clientsCount--;
                            i--;
                            j--;
                            hasDeparted = true;
                        }
                    }

                    if (hasDeparted) //if he has already departed;
                        continue;

                    clientDeparture(cs[j], waitingClientDepMin); //there were no deliveries he departs with all he can get
                    //actionHandler->onClientDepart(cs[j].index, waitingClientDepMin, cs[j].banana, cs[j].schweppes);
                    std::cout<<"cd: "<<cs[j].index<<std::endl;
                    removeClient(cs[j]);
                    clientsCount--;
                    i--;
                    j--;
                }
            }

            if(cs[i].arriveMinute <= minute) //check if the client has arrived until the given min
            {
                checkForDeliveries(cs[i].arriveMinute); //check for deliveries before the arrival

                if ((cs[i].isWaiting && clientDepTime <= minute) || checkForEnoughStock(cs[i].banana, cs[i].schweppes))
                {
                    clientDeparture(cs[i], cs[i].arriveMinute);
                    //actionHandler->onClientDepart(cs[i].index, cs[i].arriveMinute, cs[i].banana, cs[i].schweppes);
                    std::cout<<"cd: "<<cs[i].index<<std::endl;
                    removeClient(cs[i]);
                    clientsCount--;
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
                    clientDeparture(cs[i], clientDepTime);
                    //actionHandler->onClientDepart(cs[i].index, clientDepTime, cs[i].banana, cs[i].schweppes);
                    std::cout<<"cd: "<<cs[i].index<<std::endl;
                    removeClient(cs[i]);
                    clientsCount--;
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

int main()
{
    MyStore ms;
    ms.init(2, 10, 0);
    Client c{0, 10, 10, 20};
	Client c1{10, 10, 0, 0};
    Client *arr = new Client[2];
    arr[0] = c;
    arr[1] = c1;
    ms.addClients(arr, 2);
    ms.advanceTo(0);

    std::cout<<"test2\n";
    ms.advanceTo(10);

    std::cout<<"test3\n";
    ms.advanceTo(20);








    //example
    //Client c{0, 10, 0, 10};
	//Client c2{45, 35, 0, 30};
	//Client c3{46, 30, 20, 100};
	//Client c4{200, 10, 10, 1};
    //Client *arr = new Client[4];
    //arr[0] = c;
    //arr[1] = c2;
    //arr[2] = c3;
    //arr[3] = c4;
    //ms.addClients(arr, 4);
    //ms.advanceTo(200);

    //indices
    //std::cout<<"test2\n";
    //arr[0] = Client{0, 10, 0, 10};
    //ms.addClients(arr, 1);
    //ms.advanceTo(0);
//
    //std::cout<<"test3\n";
    //arr[0] = Client{0, 0, 10, 10};
    //arr[1] = Client{0, 10, 0, 3};
    //arr[2] = Client{0, 10, 10, 5};
    //ms.addClients(arr, 3);
    //ms.advanceTo(0);
//
    //std::cout<<"test4\n";
    //arr[0] = Client{0, 0, 10, 10};
    //arr[1] = Client{0, 10, 0, 3};
    //arr[2] = Client{0, 10, 10, 5};
    //ms.addClients(arr, 3);
    //ms.advanceTo(0);
}
