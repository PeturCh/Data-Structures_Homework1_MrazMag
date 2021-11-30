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

    std::vector<Client> myClients;

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
            myClients.push_back(c);
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
        for (size_t i = 0; i < myClients.size(); i++)
            if (myClients[i].index == c.index)
            {
                myClients.erase(myClients.begin() + i);
                break;
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
		for (int i = 0; i < clientsCount && !myClients.empty(); i++)
        {   
            int clientDepTime = myClients[i].arriveMinute + myClients[i].maxWaitTime;
            for (int j = 0; j < i; j++) //going threw the waiting clients
            {
                int waitingClientDepMin = myClients[j].arriveMinute + myClients[j].maxWaitTime;
                if (waitingClientDepMin <= myClients[i].arriveMinute && waitingClientDepMin <= minute) //if someone has to go before the arrival of the new client
                {
                    bool hasDeparted = false;
                    while(!arrivalTimeB.empty() && arrivalTimeB.front() <= waitingClientDepMin)//if there was delivery before he goes
                    {
                        int minAdded = addStock(ResourceType::banana);
                        if(checkForEnoughStock(myClients[j].banana, myClients[j].schweppes))
                        {
                            
                            clientDeparture(myClients[j], minAdded);
                            //actionHandler->onClientDepart(myClients[j].index, minAdded, myClients[j].banana, myClients[j].schweppes);
                            removeClient(myClients[j]);
                            hasDeparted = true;
                            clientsCount--;
                            i--;
                            j--;
                        }
                    }
                    if (hasDeparted)
                        continue;
                    
                    while(!arrivalTimeS.empty() && arrivalTimeS.front() <= waitingClientDepMin)
                    {
                        int minAdded = addStock(ResourceType::schweppes);
                        if(checkForEnoughStock(myClients[j].banana, myClients[j].schweppes))
                        {
                            
                            clientDeparture(myClients[j], minAdded);
                            //actionHandler->onClientDepart(myClients[j].index, minAdded, myClients[j].banana, myClients[j].schweppes);
                            removeClient(myClients[j]);
                            clientsCount--;
                            i--;
                            j--;
                            hasDeparted = true;
                        }
                    }

                    if (hasDeparted)
                        continue;

                    clientDeparture(myClients[j], waitingClientDepMin);
                    //actionHandler->onClientDepart(myClients[j].index, waitingClientDepMin, myClients[j].banana, myClients[j].schweppes); //NOT CORRECT WITH BANANAS AND SCHWEPPES
                    removeClient(myClients[j]);
                    clientsCount--;
                    i--;
                    j--;
                }
            }

            if(myClients[i].arriveMinute <= minute) //check if the client has arrived until the given min
            {
                checkForDeliveries(myClients[i].arriveMinute); //check for deliveries before the arrival

                if ((myClients[i].isWaiting && clientDepTime <= minute) || checkForEnoughStock(myClients[i].banana, myClients[i].schweppes))
                {
                    clientDeparture(myClients[i], myClients[i].arriveMinute);
                    //actionHandler->onClientDepart(myClients[i].index, myClients[i].arriveMinute, myClients[i].banana, myClients[i].schweppes);
                    removeClient(myClients[i]);
                    clientsCount--;
                    i--;
                    continue;
                }
                
                if(!(myClients[i].isWaiting) && myClients[i].banana > bananas + (sentForB * 100 - reservedB) && workers - (sentForS + sentForB) > 0 && !myClients[i].hasReservedB)
                {
                    std::cout<<"vlizaB\n";
                    sentForB++;
                    reservedB += myClients[i].banana;
                    myClients[i].hasReservedB = true;
                    arrivalTimeB.push(myClients[i].arriveMinute + 60);
                    std::cout << "W " << myClients[i].arriveMinute << " banana : "<< i << " index:  " << myClients[i].index << " " << minute<<std::endl;
                    //actionHandler->onWorkerSend(myClients[i].arriveMinute, ResourceType::banana);
                }

                else if (myClients[i].banana > bananas && workers - (sentForS + sentForB) > 0 && !myClients[i].hasReservedB)
                {
                    reservedB += myClients[i].banana;
                    myClients[i].hasReservedB = true;
                }
                
                if(myClients[i].schweppes > schweppes + (sentForS * 100 - reservedS) && workers - (sentForS + sentForB) > 0 && !myClients[i].isWaiting && !myClients[i].hasReservedS)
                {
                    sentForS++;
                    myClients[i].hasReservedS = true;
                    reservedS += myClients[i].schweppes;
                    arrivalTimeS.push(myClients[i].arriveMinute + 60);
                    std::cout << "W " << myClients[i].arriveMinute << " schweppes : "<< i << " " << myClients[i].index << " " << minute<<std::endl;
                    //actionHandler->onWorkerSend(myClients[i].arriveMinute, ResourceType::schweppes);
                }

                else if (myClients[i].schweppes > schweppes && (workers - (sentForS + sentForB) > 0) && !myClients[i].hasReservedS)
                {
                    reservedS += myClients[i].schweppes;
                    myClients[i].hasReservedS = true;
                }

                if (clientDepTime <= minute)
                {
                    clientDeparture(myClients[i], clientDepTime);
                    //actionHandler->onClientDepart(myClients[i].index, clientDepTime, myClients[i].banana, myClients[i].schweppes);
                    removeClient(myClients[i]);
                    clientsCount--;
                    i--;
                    continue;
                }
                myClients[i].isWaiting = true;
            }
        }
	}

	virtual int getBanana() const 
    {
		return bananas;
	}

	virtual int getSchweppes() const 
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
    Client c{0, 10, 10, 20}; // this client will get everything
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
