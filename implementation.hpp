#include "interface.h"
#include "queue.h"
#include <vector>
#include <iostream>

struct MyClient : Client //Structure which inherits Client in order to add some helping fields and constructor
{
    int arriveMinute;
	int banana;
	int schweppes;
	int maxWaitTime;
    int index;
	bool hasReservedB = false;
	bool hasReservedS = false;
	bool isWaiting = false;

	MyClient(int _arriveMin, int _banana, int _schweppes, int _maxWaitTime, int _index = 0) : arriveMinute(_arriveMin), banana(_banana), schweppes(_schweppes), maxWaitTime(_maxWaitTime)
	{
		index = _index;
	}

	MyClient(){}
};

struct MyStore : Store 
{
    bool isConsole = false;
    MyStore(bool _isConsole = false) //Constructor for the console version
    {
        isConsole = _isConsole;
    }

    int indice = 0;
    int workers = 0, bananas = 0, schweppes = 0, clientsCount = 0;

    Queue<int> arrivalMinBanana; //Arrival time of workers sent for bananas
    Queue<int> arrivalMinSchweppes; //Arrival time of workers sent for schweppes

    int reservedB = 0; //bananas reserved but not taken from the clients
    int reservedS = 0; //schweppes reserved but not taken from the clients
    int sentForB = 0; //Number of workers sent for bananas
    int sentForS = 0; //Number of workers sent for schweppes

    std::vector<MyClient> cls; //Vector of all clients

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

    //By given a resource type adds it to the stock and returns the minute the stock was added
    int addStock(ResourceType t)
    {
        if(t == ResourceType::banana)
        {
            if(!isConsole) //checking whether we have to print or update the actionHandler 
                actionHandler->onWorkerBack(arrivalMinBanana.front(), ResourceType::banana);
            else std::cout<<"D " << arrivalMinBanana.front() << " banana\n";

            bananas += 100;
            sentForB--; //A worker has returned
            return arrivalMinBanana.pop();
        }
        
        //If the resource type is schweppes
        if(!isConsole) //Checking whether we have to print or update the actionHandler 
            actionHandler->onWorkerBack(arrivalMinSchweppes.front(), ResourceType::schweppes);
        else std::cout << "D " << arrivalMinSchweppes.front() << " schweppes\n";

        schweppes += 100;
        sentForS--; //A worker has returned
        return arrivalMinSchweppes.pop();
    }

    //Checks if there are any deliveries until (including) the given minute
    //If there are some returns true and updates the stock
    bool checkForDeliveries(const int &minute)
    {
        bool hasDelivery = false;
        //If there are banana orders and they will be delivered before certain time
        while(!arrivalMinBanana.empty() && arrivalMinBanana.front() <= minute) 
        {
            addStock(ResourceType::banana);
            hasDelivery = true; 
        }
        
        //If there are schweppes orders and they will be delivered before certain time
        while(!arrivalMinSchweppes.empty() && arrivalMinSchweppes.front() <= minute)
        {
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
            MyClient c{clients[i].arriveMinute, clients[i].banana, clients[i].schweppes, clients[i].maxWaitTime, indice++};
            cls.push_back(c);
        }
	}
    /// @param bananas_ - Number of needed bananas
    /// @param schweppes_ - Number of needed schweppes
    //Checks if there is enough stock for given amount
    bool checkForEnoughStock(int &bananas_, int &schweppes_)
    {
        bool hasEnoughB = false, hasEnoughS = false;
        if(bananas_ <= bananas)
            hasEnoughB = true;

        if(schweppes_ <= schweppes)
            hasEnoughS = true;

        return hasEnoughS && hasEnoughB;
    }

    //Removes the passed client from the vector
    void removeClient(MyClient &c)
    {
        for (size_t i = 0; i < cls.size(); i++)
            if (cls[i].index == c.index)
            {
                cls.erase(cls.begin() + i);
                break;
            }
    }

    //Tries to order stock for specific client by checking if it needs to 
    //If there are enough workers and he hasn't ordered yet
    /// @param minute Minute we have advanced to
    void tryOrderSchweppes(MyClient &c, int &minute)
    {
        while(c.schweppes > schweppes + (sentForS * 100 - reservedS) && workers - (sentForS + sentForB) > 0 && !c.isWaiting && !c.hasReservedS)
        {
            sentForS++; //Increasing the number of sent workers for schweppes
            c.hasReservedS = true; //The client has reserved schweppes
            reservedS += c.schweppes; 
            arrivalMinSchweppes.enqueue(c.arriveMinute + 60); // Adding the expected arrival minute to the queue
            if(isConsole) //checking whether we have to print or update the actionHandler 
                std::cout<<"W " << c.arriveMinute << " schweppes\n";
            else
                actionHandler->onWorkerSend(c.arriveMinute, ResourceType::schweppes);
        }
        if (c.schweppes > schweppes && (workers - (sentForS + sentForB) > 0) && !c.hasReservedS) //TODO
        {
            reservedS += c.schweppes;
            c.hasReservedS = true;
        }
    }
    //Tries to order stock for specific client by checking if it needs to 
    //If there are enough workers and he hasn't ordered yet
    /// @param minute Minute we have advanced to
    void tryOrderBanana(MyClient &c, int minute)
    {
        while(c.banana > bananas + (sentForB * 100 - reservedB) && workers - (sentForS + sentForB) > 0 && !c.isWaiting && !c.hasReservedB)
        {
            sentForB++; //Increasing the number of sent workers for banana
            reservedB += c.banana; 
            c.hasReservedB = true; //The client has reserved schweppes
            arrivalMinBanana.enqueue(c.arriveMinute + 60); // Adding the expected arrival minute to the queue
            if(isConsole) //checking whether we have to print or update the actionHandler 
                std::cout<<"W " << c.arriveMinute << " banana\n";
            else
                actionHandler->onWorkerSend(c.arriveMinute, ResourceType::banana);
        }

        if (c.banana > bananas && workers - (sentForS + sentForB) > 0 && !c.hasReservedB) //TODO
        {
            reservedB += c.banana;
            c.hasReservedB = true;
        }
    }

    //Checks if there is stock to be added for the waiting client before he/she departs
    /// @param c The client we check for upcoming/enough stock
    /// @param clientDepMin The minute in which the currently processed client will depart
    /// @param currMin The minute we have advanced to 
    int checkWaitingClients(MyClient &c, const size_t &clientDepMin, int &currMin)
    {
        int waitingclientDepMin = c.maxWaitTime + c.arriveMinute; //The minute of departure of the waiting client
        if(waitingclientDepMin <= clientDepMin && waitingclientDepMin <= currMin)
        {
            while(!arrivalMinBanana.empty() && arrivalMinBanana.front() <= waitingclientDepMin)//if there were banana deliveries before he/she goes
            {
                int minAdded = addStock(ResourceType::banana); //Taking the minute the bananas has been added
                if(checkForEnoughStock(c.banana, c.schweppes)) //Checking if they are enough
                    return minAdded; //Returning the minute in which the stock is enough for the client to depart
            }

            while(!arrivalMinSchweppes.empty() && arrivalMinSchweppes.front() <= waitingclientDepMin)//If there were schweppes deliveries before he/she goes
            {
                int minAdded = addStock(ResourceType::schweppes); //Taking the minute the schweppes has been added
                if(checkForEnoughStock(c.banana, c.schweppes)) //Checking if they are enough
                    return minAdded; //Returning the minute in which the stock is enough for the client to depart
            }
            return waitingclientDepMin; //there were no deliveries he departs with all he can get
        }
        else return -1; //The client doesn't have to depart 
    }

    /// @param c The client we have to depart
    /// @param min The minute in which the client will depart
    void clientDeparture(MyClient &c, int min)
    {   
        if(c.hasReservedB) // Removing his/hers reserved bananas (if any)
            reservedB -= c.banana;

        if(c.hasReservedS) // Removing his/hers reserved schweppes (if any)
            reservedS -= c.schweppes;
            
        if (c.banana >= bananas) // Taking all the bananas if they are not enough 
        {
            c.banana = bananas;
            bananas = 0;
        }

        else
            bananas -= c.banana; // Taking just the needed bananas if there was enough in stock

        if (c.schweppes >= schweppes) // Taking all the schweppes if they are not enough 
        {
            c.schweppes = schweppes;
            schweppes = 0;
        }

        else
            schweppes -= c.schweppes; // Taking just the needed schweppes if there was enough in stock

        if(isConsole) //checking whether we have to print or update the actionHandler 
                std::cout<<c.index<< " " << min << " " << c.banana << " " << c.schweppes << std::endl;
            else
                actionHandler->onClientDepart(c.index, min, c.banana, c.schweppes);
        removeClient(c); //Removing the departed client from the vector
        clientsCount--; //decreasing the clientsCount
        
    }

	void advanceTo(int minute) override  
    {
		for (int i = 0; i < clientsCount && !cls.empty(); i++) //Going threw all the clients
        {   
            int MyClientDepTime = cls[i].arriveMinute + cls[i].maxWaitTime; //Current client departure time
            for (int j = 0; j < i; j++) //Going threw the waiting clients who came before the current
            {
                if(checkWaitingClients(cls[j], MyClientDepTime, minute) != -1)
                {
                    clientDeparture(cls[j], MyClientDepTime);
                    --i;
                    --j;
                }
            }

            if(cls[i].arriveMinute <= minute) //check if the the current client has arrived until the given min
            {
                checkForDeliveries(cls[i].arriveMinute); //check for deliveries before the arrival

                //Check for whether he/she has already ordered and has to depart or there is enough stock for him/her to depart
                if ((cls[i].isWaiting && MyClientDepTime <= minute) || checkForEnoughStock(cls[i].banana, cls[i].schweppes))
                {
                    clientDeparture(cls[i], cls[i].arriveMinute);
                    i--;
                    continue; //Going to the next client in the vector
                }

                //Edge case for only one worker in the store, ordering the more needed stock or the preferred stock (banana) when there are equal needs
                if(workers - (sentForB + sentForS) == 1 && cls[i].banana != 0 && cls[i].schweppes != 0 && bananas < cls[i].banana && schweppes < cls[i].schweppes)
                {
                    if(cls[i].schweppes - schweppes <= cls[i].banana - bananas)
                        tryOrderBanana(cls[i], minute);
                            else tryOrderSchweppes(cls[i], minute);
                }

                //Trying to order stock for the current client
                tryOrderBanana(cls[i], minute);
                tryOrderSchweppes(cls[i], minute);
                cls[i].isWaiting = true; //The client is now waiting

                //If the client has ordered but has to depart before the arrival of the order
                if (MyClientDepTime <= minute) 
                {
                    MyClient curr = cls[i];
                    //Going threw the waiting clients if the current had pushed someone in front of him/her 
                    for (size_t j = i + 1; j < clientsCount; j++) 
                    {
                        //If there weren't any arrivals of stock the client departs
                        if(checkWaitingClients(cls[j], MyClientDepTime, minute) != -1) 
                        {
                            clientDeparture(cls[j], MyClientDepTime);
                            --i;
                            --j;
                        }
                        //Check if someone is arriving before the departure of the current and:
                        //1. Can depart before the current if there is enough stock
                        //2. Should place order before the current departure
                        else if(cls[j].arriveMinute < MyClientDepTime)
                        {
                            if(checkForEnoughStock(cls[j].banana, cls[j].schweppes))
                                clientDeparture(cls[j], cls[j].arriveMinute);
                            else if(!cls[j].isWaiting)
                            {
                                tryOrderBanana(cls[j], minute);
                                tryOrderSchweppes(cls[j], minute);
                            }
                        }
                    }
                    
                    //Variable validating if the current client has departed
                    bool hasDeparted = false;

                    //Checking for banana arrivals before the departure
                    while(!arrivalMinBanana.empty() && arrivalMinBanana.front() <= MyClientDepTime)
                    {
                        //Saving the last minute in which stock was added
                        int minuteAdded = addStock(ResourceType::banana);
                        if(checkForEnoughStock(curr.banana, curr.schweppes)) //Checking if the stock is enough already
                        {
                            clientDeparture(curr, minuteAdded); //Deperts the client when the products has been added
                            hasDeparted = true;
                            break;
                        }
                    }

                    //If the client has departed we don't have to check for schweppes
                    if(hasDeparted)
                    {
                        i--;
                        continue;
                    }

                    //Checking for schweppes arrivals before the departure
                    while(!arrivalMinSchweppes.empty() && arrivalMinSchweppes.front() < MyClientDepTime)
                    {
                        //Saving the last minute in which stock was added
                        int minuteAdded = addStock(ResourceType::schweppes);
                        if(checkForEnoughStock(curr.banana, curr.schweppes)) //Checking if the stock is enough already
                        {
                            clientDeparture(curr, minuteAdded); //Deperts the client when the products has been added
                            hasDeparted = true;
                            break;
                        }
                    }
                    
                    //If there weren't any deliveries until the deprture time the client departs after the waiting time
                    if(!hasDeparted)
                        clientDeparture(curr, MyClientDepTime);

                    i--;
                    continue;
                }

                cls[i].isWaiting = true; //The client shouldn't departs still so he is waiting
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