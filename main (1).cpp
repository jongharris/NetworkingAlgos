//Jonathan Harris
//30062368
//CPSC 441 Assignment 3

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <stack>
#include <algorithm>
using namespace std;

#define MAX_EVENTS 10
#define CALL_START 1
#define CALL_END 0
#define V 26

int propdelay[V][V];
int capacity[V][V];
int available[V][V];
float cost[V][V];
double hops = 0;
double delayAvg = 0;
double hopsToCompare = 0;
double hopsEmpty = 0;

int minDistance(float[], bool[]);
int maxDistance(float[], bool[]);
bool RouteCall(char, char, int, stack<int> &, int);
stack<int> djikstras(float[][V], char, char);
stack<int> djikstrasLLP(float[][V], char, char);
stack<int> djikstrasMFC(float[][V], char, char);
void constructGraph(int);
void releaseCall(char, char, stack<int> &);
void printResults(int, int, int, double, double, double, double, int);

//Structure to hold events
class Node
{
public:
    float event_time;
    int event_type;
    int callid;
    char source;
    char destination;
    float duration;
    stack<int> path;
    Node *next;
};

int main()
{
    char src, dst;
    int delay, cap, row, col, numevents, i, success = 0, blocked = 0, eCount = 0, numCalls = 0;
    float start, duration;
    FILE *fp1;
    FILE *fp2;
    fp1 = fopen("topology.dat", "r");

    while ((i = fscanf(fp1, "%c %c %d %d\n", &src, &dst, &delay, &cap)) == 4)
    {
        row = src - 'A';
        col = dst - 'A';
        propdelay[row][col] = delay;
        propdelay[col][row] = delay;
        capacity[row][col] = cap;
        capacity[col][row] = cap;
        available[row][col] = cap;
        available[col][row] = cap;
    }
    fclose(fp1);

    /* Next read in the calls from "callworkload.dat" and set up events */
    fp2 = fopen("callworkload.dat", "r");

    /* Linked List to store input */
    Node *head = new Node();
    Node *temp = head;
    temp->next = NULL;

    if ((i = fscanf(fp2, "%f %c %c %f\n", &start, &src, &dst, &duration)) == 4)
        while (1)
        {
            temp->next = NULL;
            temp->event_time = start;
            temp->source = src;
            temp->destination = dst;
            temp->duration = duration;
            temp->event_type = CALL_START;
            temp->callid = eCount;

            if ((i = fscanf(fp2, "%f %c %c %f\n", &start, &src, &dst, &duration)) == 4)
            {
                temp->next = new Node();
                temp = temp->next;
            }
            else
            {
                break;
            }
            eCount++;
        }

    /* Now simulate the call arrivals and departures */
    eCount = 0;
    Node *current = head;

    //Init a stack to contain the path
    stack<int> path;

    //Set previous initally to be pointed to the head of the list
    Node *previous = head;

    //Printing the display
    cout << "Algorithm \tCalls \tSuccess (%) \tBlocked (%) \tAvgHops \tAvgDelay " << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    for (int s = 0; s < 4; s++)
    {
        current = head;
        hops = 0;
        success = 0;
        blocked = 0;
        delayAvg = 0;
        numCalls = 0;

        while (current != NULL)
        {
            if (current->event_type == CALL_START)
            {
                numCalls++;
                if (RouteCall(current->source, current->destination, current->callid, path, s))
                {
                    //  cout << current->event_time << " " << current->callid << endl;

                    success++;
                    //insert the endtime as a new event
                    Node *endEvent = new Node();
                    endEvent->event_time = current->event_time + current->duration;
                    endEvent->source = current->source;
                    endEvent->destination = current->destination;
                    endEvent->event_type = CALL_END;
                    endEvent->callid = current->callid;
                    endEvent->duration = -1;
                    endEvent->next = NULL;
                    endEvent->path = path;

                    Node *traverse = current;
                    while (traverse->next != NULL && traverse->next->event_time < endEvent->event_time)
                    {
                        traverse = traverse->next;
                    }
                    endEvent->next = traverse->next;
                    traverse->next = endEvent;
                }
                else
                {
                    //   cout << "BLOCKED " << current->event_time << " " << current->callid << endl;
                    blocked++;
                }
            }
            else
            {
                //release the call CALL_END
                stack<int> tempPath = current->path;
                releaseCall(current->source, current->destination, tempPath);

                //delete the end event for other algorithms;
                if (current->next == NULL)
                {
                    previous->next = NULL;
                }
                else
                {
                    previous->next = current->next;
                    free(current);
                    current = previous;
                }
            }

            eCount++;
            previous = current;
            current = current->next;
        }

        /* Print final report here */
        printResults(s, success, blocked, (double)success / numCalls, (double)blocked / numCalls, hops / success, delayAvg / success, numCalls);
    }
}
void printResults(int type, int success, int blocked, double successP, double blockedP, double avgHops, double avgDelay, int calls)
{

    if (type == 0)
    {
        cout << "SHPF \t\t" << calls << " \t" << success << " (" << successP * 100 << ") \t" << blocked << " (" << blockedP * 100 << ") \t" << avgHops << " \t" << avgDelay << endl;
    }
    else if (type == 1)
    {
        cout << "SDPF \t\t" << calls << " \t" << success << " (" << successP * 100 << ") \t" << blocked << " (" << blockedP * 100 << ") \t" << avgHops << " \t" << avgDelay << endl;
    }
    else if (type == 2)
    {
        cout << "LLP \t\t" << calls << " \t" << success << " (" << successP * 100 << ") \t" << blocked << " (" << blockedP * 100 << ") \t" << avgHops << " \t" << avgDelay << endl;
    }
    else
    {
        cout << "MFC \t\t" << calls << " \t" << success << " (" << successP * 100 << ") \t" << blocked << " (" << blockedP * 100 << ") \t" << avgHops << " \t" << avgDelay << endl;
    }
}

bool RouteCall(char src, char dst, int callid, stack<int> &path, int type)
{
    bool success = true;

    //constructs the graph for djikstras in global array cost
    constructGraph(type);

    //Chooses the correct djikstras for the algorithm
    if (type == 0 || type == 1)
    {
        path = djikstras(cost, src, dst);
    }
    else if (type == 2)
    {
        path = djikstrasLLP(cost, src, dst);
    }
    else
    {
        path = djikstrasMFC(cost, src, dst);
    }

    //variables to hold the path, two at a time
    int next;
    int current;

    //hold a temp stack as the other stack will be emptied
    stack<int> temp;
    temp = path;
    //if a path was never initialized, then don't change the availability.
    if (!path.empty())
    {
        while (1)
        {

            current = path.top();
            path.pop();

            //uncomment the below cout to see the path
            //cout << current << ' ';

            //now peek at the next to find the link
            if (!path.empty())
            {

                hops++;
                hopsToCompare++;
                next = path.top();
                delayAvg += propdelay[current][next];

                //now set the availability of the vertexes
                available[current][next]--;
                available[next][current]--;
            }
            else
            {
                break;
            }
        }

        //uncomment the cout to see the path with endlines
        //cout << endl;

        path = temp;
    }
    else
    {
        success = false;
    }

    return success;
}

void constructGraph(int type)
{

    //Construct the correct cost graph based on the type passed
    for (int i = 0; i < V; i++)
    {
        for (int k = 0; k < V; k++)
        {
            if (available[i][k] > 0)
            {
                if (type == 0)
                {
                    cost[i][k] = 1;
                }
                else if (type == 1)
                {
                    cost[i][k] = propdelay[i][k];
                }
                else if (type == 2)
                {
                    cost[i][k] = 1;
                }
                else if (type == 3)
                {
                    cost[i][k] = 1;
                }
                else
                {
                    cost[i][k] = 1;
                }
            }
            else
            {
                cost[i][k] = 0;
            }
        }
    }
}

//Djikstras code built from Wikipedia Djikstras pseudocode and
//https://www.geeksforgeeks.org/dijkstras-shortest-path-algorithm-greedy-algo-7/ as reference
stack<int> djikstras(float graph[V][V], char src, char dst)
{

    float dist[V];
    bool setPath[V];
    int previous[V];

    for (int i = 0; i < V; i++)
    {
        dist[i] = INT_MAX;
        setPath[i] = false;
        previous[i] = INT_MAX;
    }

    dist[src - 'A'] = 0;

    //find the path from src to dst and update the availabilities
    for (int count = 0; count < V - 1; count++)
    {
        int u = minDistance(dist, setPath);

        setPath[u] = true;

        //early exit code
        //       // if U is the DST then early exit
        //        if (u == dst - 'A') {
        //            break;
        //        }

        for (int v = 0; v < V; v++)
        {
            if (!setPath[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v])
            {
                previous[v] = u;
                dist[v] = dist[u] + graph[u][v];
            }
        }
    }

    stack<int> path;
    int u = dst - 'A';

    //if the destination was found then build the path
    if (previous[u] != INT_MAX || u == src - 'A')
    {
        while (u != INT_MAX)
        {
            path.push(u);
            u = previous[u];
        }
    }

    return path;
}

//Djikstras function for LLP.
//Sets path dynamically rather than using the initial cost graph
stack<int> djikstrasLLP(float graph[V][V], char src, char dst)
{

    float dist[V];
    bool setPath[V];
    int previous[V];

    for (int i = 0; i < V; i++)
    {
        dist[i] = INT_MAX;
        setPath[i] = false;
        previous[i] = INT_MAX;
    }

    dist[src - 'A'] = 0;

    //find the path from src to dst and update the availabilities
    for (int count = 0; count < V - 1; count++)
    {
        int u = minDistance(dist, setPath);

        setPath[u] = true;

        //calculate the paths dynamically...

        for (int v = 0; v < V; v++)
        {
            float utilization = 0;

            //if there is currently available calls on the line
            if (available[u][v])
                utilization = (1.0 - ((float)available[u][v] / (float)capacity[u][v]));

            //to check we go
            float middleCost = max(dist[u], utilization);

            if (!setPath[v] && graph[u][v] && dist[u] != INT_MAX && middleCost < dist[v])
            {
                dist[v] = middleCost;
                previous[v] = u;
            }
        }
    }

    stack<int> path;
    int u = dst - 'A';
    // cout << " The destination is: " << dst - 'A' << endl;
    //if the destination was found then go into this
    if (previous[u] != INT_MAX || u == src - 'A')
    {
        while (u != INT_MAX)
        {
            path.push(u);
            u = previous[u];
        }
    }

    return path;
}

//Djikstras for MFC
//Sets costs dynamically instead of initial cost graph.
stack<int> djikstrasMFC(float graph[V][V], char src, char dst)
{

    float dist[V];
    bool setPath[V];
    int previous[V];

    for (int i = 0; i < V; i++)
    {
        dist[i] = -1;
        setPath[i] = false;
        previous[i] = -1;
    }

    dist[src - 'A'] = 0;

    //find the path from src to dst and update the availabilities
    for (int count = 0; count < V; count++)
    {
        int u = maxDistance(dist, setPath);

        setPath[u] = true;

        for (int v = 0; v < V; v++)
        {
            float MFC = -10;

            //if dist[v] has not been initialized (it is currently -1), then set it to the current capacity between u and v
            if (dist[v] < 0)
            {
                MFC = available[u][v];
            }
            else if (available[u][v])
            {
                MFC = min(dist[u], (float)available[u][v]);
            }

            if (!setPath[v] && graph[u][v] && dist[u] != -1 && MFC > dist[v])
            {
                dist[v] = MFC;
                previous[v] = u;
            }
        }
    }

    stack<int> path;
    int u = dst - 'A';

    //if the destination was found then go into this
    if (previous[u] != -1 || u == src - 'A')
    {
        while (u != -1)
        {
            path.push(u);
            u = previous[u];
        }
    }

    return path;
}

//Function to give back available capacity to edges along the path
void releaseCall(char src, char dst, stack<int> &path)
{

    int next;
    int current;

    if (!path.empty())
    {
        while (1)
        {
            current = path.top();
            path.pop();
            //now peek at the next to find the link
            if (!path.empty())
            {
                next = path.top();
                //now set the availability of this vertex
                available[current][next]++;
                available[next][current]++;
            }
            else
            {
                break;
            }
        }
    }
}

int minDistance(float dist[], bool shortest[])
{
    //finds the minimum vertex that have not yet been visited.
    float min = INT_MAX;
    int min_index;
    for (int v = 0; v < V; v++)
    {
        if (shortest[v] == false && dist[v] <= min)
        {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

int maxDistance(float dist[], bool shortest[])
{
    //finds the maximum vertex that have not yet been visited.
    float max = -1;
    int max_index;
    for (int v = 0; v < V; v++)
    {
        if (shortest[v] == false && dist[v] >= max)
        {
            max = dist[v];
            max_index = v;
        }
    }
    return max_index;
}
