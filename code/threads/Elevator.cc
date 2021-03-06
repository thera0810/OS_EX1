#include "Elevator.h"

extern Thread* currentThread;
extern Alarm* alarms;
extern int floors;
extern int capacity;

Elevator::Elevator(char* debugName, int numFloors, int myID)
{
	name=debugName;
	currentfloor=1;//start floor is 1
	occupancy=0;
	floorCounts=numFloors;
	elevatorID=myID;
	dir=0;
	// open=0;
    int i;
    for (i=1;i<=numFloors;i++)
    {
    	floorCalled[i] = 0;
    	exitBar[i] = new EventBarrier();
    }
    building = NULL;
}

Elevator::~Elevator()
{
    int i;
    for (i=1;i<=floorCounts;i++)
		delete exitBar[i]; 
}

void Elevator::getelevID()//record the elevator that arrive floor, then rider try to enter it
{
    if (dir == 1)
        building->GetUpID(currentfloor, elevatorID);
    else // direction == DOWN
        building->GetDownID(currentfloor, elevatorID);
}

void Elevator::OpenDoors()//   signal exiters and enterers to action
{
	DEBUG('E',"\033[1;32;40mElevator %d OpenDoors on %d floor.\033[m\n\n", elevatorID, currentfloor);

	if (exitBar[currentfloor]->Waiters()>0)
		exitBar[currentfloor]->Signal();

	getelevID();

	if (building->enterBarUp[currentfloor]->Waiters()>0&&dir==1)// move up and rider request up
		building->enterBarUp[currentfloor]->Signal();

	if(building->enterBarDown[currentfloor]->Waiters()>0&&dir==0)
		building->enterBarDown[currentfloor]->Signal();
}

void Elevator::CloseDoors()//   after exiters are out and enterers are in
{
	currentThread->Yield();//to get all riders' request and do better visit
	DEBUG('E',"\033[1;32;40mElevator %d CloseDoors on %d floor.\033[m\n\n", elevatorID, currentfloor);
}

void Elevator::VisitFloor(int floor)//   go to a particular floor
{
	if (floor>currentfloor)
	{
		DEBUG('E',"Elevator %d is operating for %d Time Unit .\n\n", elevatorID, 10*(floor-currentfloor));
		alarms->Pause(10*(floor-currentfloor));
	}
	else
	{
		DEBUG('E',"Elevator %d is operating for %d Time Unit .\n\n", elevatorID, 10*(currentfloor-floor));
		alarms->Pause(10*(currentfloor-floor));
	}

	currentfloor=floor;
	floorCalled[floor]=0;//reset buttom

	DEBUG('E',"\033[1;32;40mElevator %d arrived %d floor.\033[m\n", elevatorID, currentfloor);
}

bool Elevator::Enter()//   get in
{
	building->riderRequest--;//assume this rider's request has been satisfied
	ASSERT(building->riderRequest>=0);

	if (occupancy<capacity)//can enter successfully
	{
		occupancy++;
		DEBUG('E',"\033[1;33;40mRider %s enter elevator %d on (%d) floor.\033[m\n",currentThread->getName(),elevatorID,currentfloor);
		if(dir==1){
			building->floorCalledUp[currentfloor]=0;
			building->enterBarUp[currentfloor]->Complete();
		}
		else{
			building->floorCalledDown[currentfloor]=0;
			building->enterBarDown[currentfloor]->Complete();
		}
		return true;
	}
	else// Overload!
	{
		DEBUG('E',"\033[1;31;40mFULL!! Rider %s can not enter elevator %d on (%d) floor.\033[m\n\n",currentThread->getName(),elevatorID,currentfloor);
		if(dir==1){
			building->floorCalledDown[currentfloor]=0;
			building->enterBarUp[currentfloor]->Complete();
		}
		else{
			building->floorCalledDown[currentfloor]=0;
			building->enterBarDown[currentfloor]->Complete();
		}
		currentThread->Yield();//let elevator change state of EnterBar
		return false;
	}
}

void Elevator::Exit()//   get out (iff destinationFloor)
{
	occupancy--;
	DEBUG('E',"\033[1;33;40mRider %s exit elevator %d on (%d) floor.\033[m\n\n",currentThread->getName(),elevatorID,currentfloor);
	exitBar[currentfloor]->Complete(); // complete get out
}

void Elevator::RequestFloor(int floor)//   tell the elevator our destinationFloor
{
	floorCalled[floor]=1;//press the buttom inside the elevator
	DEBUG('E',"\033[1;33;40mRider %s RequestFloor(%d)\033[m\n\n",currentThread->getName(), floor);
	exitBar[floor]->Wait(); // wait to get out
}

int Elevator::noneedUp(int here){//if there are need to go up
	int flag=1;
	for(int i=here+1;i<=floorCounts;++i){
		if (building->floorCalledUp[i]==1||floorCalled[i]==1)
			{
				flag=0;
				break;
			}
	}
	return flag;
}

int Elevator::noneedDown(int here){//if there are need to go down
	int flag=1;
	for(int i=here-1;i>=1;--i){
		if (building->floorCalledDown[i]==1||floorCalled[i]==1)
			{
				flag=0;
				break;
			}
	}
	return flag;
}

void Elevator::Operating()//   elevator operating loop
{
	DEBUG('E',"\033[1;32;40mElevator %d start working\033[m\n\n",elevatorID);
	while (true)//main loop
	{
		if (dir==1) //UP
		{
			for (int i=currentfloor+1;i<=floorCounts;i++)
			{
				if (building->floorCalledUp[i]==1||floorCalled[i]==1) //need to go i floor
				{
					VisitFloor(i);
					OpenDoors();

					if(noneedUp(i)){//no need to up but still have the need to pick down-request rider
						if(building->floorCalledDown[i]==1){	//this floor has down request
							dir = 0;							//then turn direction and pick the down-request rider
							building->floorCalledDown[currentfloor] = 0;
							if(building->enterBarDown[currentfloor]->Waiters()>0)
								building->enterBarDown[currentfloor]->Signal();
						}
						CloseDoors();
						break;// break to check down request
					}
					else
						CloseDoors(); //whenever closedoor is needed

				}
			}
			for(int i=floorCounts;i>=1;--i){//check down request
				if(building->floorCalledDown[i]==1){
					VisitFloor(i);
					dir=0;
					OpenDoors();
					CloseDoors();
					break;//break to go back to down direction loop
				}
			}
			
			dir=0;//in the end, change direction is needed. to pick fail-enterd rider
		}
		else if (dir==0)//DOWN, same as UP scenario
		{
			for (int i=currentfloor-1;i>=1;i--)
			{
				if (building->floorCalledDown[i]==1||floorCalled[i]==1)
				{
					VisitFloor(i);
					OpenDoors();

					if(noneedDown(i)){
						if(building->floorCalledUp[i]==1){
							dir = 1;
							building->floorCalledUp[currentfloor] = 0;
							if(building->enterBarUp[currentfloor]->Waiters()>0)
								building->enterBarUp[currentfloor]->Signal();
						}
						CloseDoors();
						break;
					}
					else
						CloseDoors();
				}
			}
			for(int i=1;i<=floorCounts;++i){
				if(building->floorCalledUp[i]==1){
					VisitFloor(i);
					dir=1;
					OpenDoors();
					CloseDoors();
					break;
				}
			}
			dir=1;
		}

		building->lock->Acquire();
		while(building->riderRequest==0&&occupancy==0){//if no request and task, then sleep the elevator
			DEBUG('E',"\033[1;32;40mNo task or request, elevator %d sleep\033[m\n\n",elevatorID);
			building->cond->Wait(building->lock);
			DEBUG('E',"\033[1;32;40mElevator %d start working\033[m\n\n",elevatorID);
		}
		building->lock->Release();
	}
}

void Elevator::SetBuilding(Building *b)
{
    building = b;
}

//******************************building ****************************************

Building::Building(char *debugname, int numFloors, int numElevators)
{
	name=debugname;
	floorCounts=numFloors;
	elevatorCounts=numElevators;
    elevatorUpID = new int[numFloors];
    elevatorDownID = new int[numFloors];
    upIDLock = new Lock("upIDLock");
    downIDLock = new Lock("downIDLock");
    lock = new Lock("Elevator lock");
    cond = new Condition("Elevator condition");
    riderRequest=0;

    //create elevators of number of numElevators
    elevator = (Elevator *)::operator new[](sizeof(Elevator) * numElevators);
    for (int i = 0; i < numElevators; i++)
    {
        new (&elevator[i]) Elevator("BuildingElevator", numFloors, i);
        elevator[i].SetBuilding(this);
    }

    //init buttom and barrier
    int i;
    for (i=1;i<=numFloors;i++)
    {
    	floorCalledUp[i]=0;
    	floorCalledDown[i]=0;
    	enterBarUp[i] = new EventBarrier();
    	enterBarDown[i] = new EventBarrier();
    }
}

Building::~Building()
{
	delete[] elevatorUpID;
    delete[] elevatorDownID;
    delete upIDLock;
    delete downIDLock;
    delete lock;
    delete cond;
    int i;
    for (i=1;i<=floorCounts;i++)
    {
		delete enterBarUp[i];
		delete enterBarDown[i];
    }
    for (i = 0; i < elevatorCounts; i++)
    {
        elevator[i].~Elevator();
    }
    ::operator delete[](elevator);
}

void Building::CallUp(int fromFloor)//   signal an elevator we want to go up
{
	riderRequest++;
	floorCalledUp[fromFloor]=1;//press floor's up buttom
	
	lock->Acquire();
	cond->Broadcast(lock);
	lock->Release();
	
}

void Building::CallDown(int fromFloor)//   signal an elevator we want to go down
{
	riderRequest++;
	floorCalledDown[fromFloor]=1;
	
	lock->Acquire();
	cond->Broadcast(lock);
	lock->Release();
	
}

Elevator* Building::AwaitUp(int fromFloor)// wait for elevator arrival & going up
{
	enterBarUp[fromFloor]->Wait();
	return elevator + elevatorUpID[fromFloor];//try to enter the last elevator that change elevatorUpID[]
}

Elevator* Building::AwaitDown(int fromFloor)// wait for elevator arrival & going down
{
	enterBarDown[fromFloor]->Wait();
	return elevator + elevatorDownID[fromFloor];
}

void Building::StartElevator()// tell elevator to operating forever
{
	elevator->Operating();
}

void Building::GetUpID(int floor, int elevatorID)
{
    upIDLock->Acquire();
    elevatorUpID[floor] = elevatorID;
    upIDLock->Release();
}

void Building::GetDownID(int floor, int elevatorID)
{
    downIDLock->Acquire();
    elevatorDownID[floor] = elevatorID;
    downIDLock->Release();
}

Elevator *Building::GetElevator()
{
    return elevator;
}