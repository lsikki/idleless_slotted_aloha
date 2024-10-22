#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

using namespace ns3;

int main(int argc, char* argv[]) {

    // Here are are creating the nodes.
    srand((unsigned) time(NULL)); // seed for the random number generation
    NodeContainer nodes;
    int randomNumberOfNodes = 5 + (rand() % 96); // The paper had between 5-100 nodes for the simulation
    nodes.Create(randomNumberOfNodes);
    std::cout << "Number of nodes generated:" << nodes.GetN() << std::endl;

    // The first node will be the ground control station (gcs)
    NodeContainer gcs = NodeContainer(nodes.Get(0));
    std::cout << "Number of ground control stations:" << gcs.GetN() << std::endl;

    // The rest of the nodes will be the drones
    NodeContainer drones;
    for (uint32_t i = 1; i < nodes.GetN(); ++i) {
        drones.Add(nodes.Get(i));
    }
    std::cout << "Number of drones:" << drones.GetN() << std::endl;

    // Here we will establish a shared medium between all nodes
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(100000))); // Paper uses 100 kbps data transmission rate
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    NetDeviceContainer connectedDevices = csma.Install(nodes);

    // Next steps
    // Make the drones mobile and the ground control station stationary
    // Install internet connections and establish IP addresses
    // Implement p-persistent sensing strategy
    // Add slots
    // Add ID transmission and ACK sending functionalities
    // Add idle slot detection

    // Simulator::Run();
    // Simulator::Destroy();

    return 0;
}
