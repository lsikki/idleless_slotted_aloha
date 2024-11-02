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
#include <vector>
#include <random>

using namespace ns3;

class Drone : public Application {
public:
    Drone(Ptr<Socket> socket, uint32_t id) : m_socket(socket), m_id(id), m_transmitting(false) {}

    void StartApplication() override {
        m_socket->Bind();
        m_socket->SetRecvCallback(MakeCallback(&Drone::ReceiveAck, this));
        ScheduleNextTransmission();
    }

    void ReceiveAck(Ptr<Socket> socket) {
        Address from;
        Ptr<Packet> packet = socket->RecvFrom(from);
        std::cout << "Drone " << m_id << " received ACK from GCS." << std::endl;
        m_transmitting = false;
        ScheduleNextTransmission();
    }

    void SendData() {
        if (!m_transmitting) {
            m_transmitting = true;
            Ptr<Packet> packet = Create<Packet>(m_id);
            m_socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("10.1.1.1"), 9)); // Assume GCS IP is 10.1.1.1
            std::cout << "Drone " << m_id << " sent data." << std::endl;
        }
    }

    void ScheduleNextTransmission() {
        Time nextTransmission = Seconds(1.0); // Set your time slot duration
        Simulator::Schedule(nextTransmission, &Drone::SendData, this);
    }

private:
    Ptr<Socket> m_socket;
    uint32_t m_id;
    bool m_transmitting;
};

class GroundControlStation : public Application {
public:
    GroundControlStation(Ptr<Socket> socket) : m_socket(socket) {}

    void StartApplication() override {
        m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
        m_socket->SetRecvCallback(MakeCallback(&GroundControlStation::ReceiveData, this));
    }

    void ReceiveData(Ptr<Socket> socket) {
        Address from;
        Ptr<Packet> packet = socket->RecvFrom(from);
        uint32_t droneId;
        packet->CopyData((uint8_t*)&droneId, sizeof(droneId));
        std::cout << "GCS received data from Drone " << droneId << std::endl;
        SendAck(droneId);
    }

    void SendAck(uint32_t droneId) {
        Ptr<Packet> ackPacket = Create<Packet>(droneId);
        m_socket->SendTo(ackPacket, 0, InetSocketAddress(Ipv4Address("10.1.1." + std::to_string(droneId)), 9));
        std::cout << "GCS sent ACK to Drone " << droneId << std::endl;
    }

private:
    Ptr<Socket> m_socket;
};

int main(int argc, char* argv[]) {
    srand((unsigned)time(NULL)); // Seed for random number generation

    NodeContainer nodes;
    int randomNumberOfNodes = 5 + (rand() % 96); // Between 5-100 nodes
    nodes.Create(randomNumberOfNodes);
    std::cout << "Number of nodes generated: " << nodes.GetN() << std::endl;

    // Ground control station
    NodeContainer gcs = NodeContainer(nodes.Get(0));
    std::cout << "Number of ground control stations: " << gcs.GetN() << std::endl;

    // Drones
    NodeContainer drones;
    for (uint32_t i = 1; i < nodes.GetN(); ++i) {
        drones.Add(nodes.Get(i));
    }
    std::cout << "Number of drones: " << drones.GetN() << std::endl;

    // Establish a shared medium
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(100000))); // 100 kbps
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer connectedDevices = csma.Install(nodes);

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(connectedDevices);

    // Create and install GCS application
    Ptr<Socket> gcsSocket = Socket::CreateSocket(nodes.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    GroundControlStation gcsApp(gcsSocket);
    nodes.Get(0)->AddApplication(&gcsApp);
    gcsApp.SetStartTime(Seconds(1.0));

    // Create and install drone applications
    for (uint32_t i = 1; i < nodes.GetN(); ++i) {
        Ptr<Socket> droneSocket = Socket::CreateSocket(nodes.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
        Drone droneApp(droneSocket, i);
        nodes.Get(i)->AddApplication(&droneApp);
        droneApp.SetStartTime(Seconds(1.0));
    }

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}