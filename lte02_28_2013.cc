/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/radio-bearer-stats-calculator.h"
#include "ns3/mac-stats-calculator.h"
#include <iomanip>
#include "ns3/flow-monitor-module.h"
#include <string.h>
#include <iostream>
#include <utility>
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-helper.h"
#include <ctime>
#include <map>
#include <fstream>

using namespace ns3;


FlowMonitorHelper flowMonHelper;
Ptr<FlowMonitor> flowmon;
std::ostringstream tag;
std::ostringstream prefixString;
int flowCount = 0;
int TxByteSum = 0;
int RxByteSum = 0;
int TxPacketsSum = 0;
int RxPacketsSum = 0;
int LostPackets = 0;
double PktLostRatio = 0;
double ThroughputAverage = 0;
double DelayAverage = 0; 
double JitterAverage = 0;
double enbDist = 100.0;
double radius = 50.0;
int numUes = 10;
int isComp = 0;
int sec = 1;
int isFlat = 0;
int isAllot = 0;
int nodeBApplication[4];
char *pathValue;
std::string pathOut;
int bitrate[4];

std::string GetNApplication(Ptr<Node> node)
{
  std::ostringstream result;
  result<<node->GetNApplications()<<"\t";
  for(uint32_t  i=0 ; i< node->GetNApplications() ; i++)
  {
    result<<node->GetApplication(i)->GetTypeId().GetName()<<"\n";
  }
  return result.str();    
}


std::string GetNodePlace(Ptr<Node> node)
{
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
  Vector pos = mob->GetPosition (); 
  std::ostringstream result;
  result <<"\t"<<pos.x << "\t" << pos.y << std::endl;
  return result.str();    
}

std::string GetIp(Ptr<Node> node)
{
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
        Ipv4Address addri = iaddr.GetLocal (); 
        std::ostringstream result;
        result << addri;
        return result.str();
}

void PrintOneNotePlace(Ptr<Node> node)
{
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
  Vector pos = mob->GetPosition ();
  std::cout <<pos.x << "\t" << pos.y << std::endl;
}

void PrintNodePlace(NodeContainer node, int numUes)
{
	  for(int i=0; i<numUes; i++)
		  {
       /* Ptr<Ipv4> ipv4 = node.Get (i)->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
        Ipv4Address addri = iaddr.GetLocal (); */
        std::cout<<GetIp(node.Get(i))<<"\t"<<GetNodePlace(node.Get(i))<<"\n";
        
		  }
}

void ExportGnuFile(NodeContainer nodeB, NetDeviceContainer ueNodes[4])
{
        std::string pathOut2 = pathOut + "/Gnu/";
        pathOut2.append(tag.str ());
        pathOut2.append(".dat");
        char *fileName = (char*)pathOut2.c_str();    
          std::ofstream out( fileName );

        if( !out )
        {
           std::cout << "Couldn't open file."  << std::endl;
        }
        else
        {
          out<<"# Data to generate gnuplot graph from position\n";
          out<<"# Copyright Ygal Bellaiche\n";
          out<<"# Base1\n";
          out<<GetNodePlace(nodeB.Get(0));
          out<<"\n\n";
          out<<"# Base2\n";
          out<<GetNodePlace(nodeB.Get(1));
          out<<"\n\n";
          out<<"# Base3\n";
          out<<GetNodePlace(nodeB.Get(2));
          out<<"\n\n";
          out<<"# Base4\n";
          out<<GetNodePlace(nodeB.Get(3));
          out<<"\n\n";
          for(int w = 0 ; w < 4; w++)
          {
            int countInNodeb = 0;
              for(int v= 0; v <4 ; v++)
              {
                  for(uint i=0; i<ueNodes[v].GetN(); i++)
                  {                    
                    Ptr<Node> enbNode = ueNodes[v].Get (i)->GetObject<LteUeNetDevice>()->GetTargetEnb()->GetNode();
                    std::string nodeBstr = GetIp(enbNode);
                    std::string nodeBCandstr = GetIp(nodeB.Get(w));
                    if(nodeBstr.compare(nodeBCandstr) == 0)
                    {
                      out<<GetNodePlace(ueNodes[v].Get (i)-> GetNode());
                      countInNodeb++;
                    }
                  }
              }
              if(countInNodeb) // empty case
                  {
                    out<<GetNodePlace(nodeB.Get(w));
                  }
              out<<"\n\n";
          } 
        }
}


void PrintNodeApplication(NodeContainer node, int numUes)
{
  for(int v = 0 ; v < numUes ; v++)
  {
   // std::cout<<v<<"    "<<(node.Get (v)->GetApplication(0)->GetTypeId()).GetName()<<"\n";
  }
}

void PrintNodeIP(NodeContainer node, int numUes)
{
  /*for(int v = 0 ; v < numUes ; v++)
  {
        Ptr<Ipv4> ipv4 = node.Get (v)->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
        Ipv4Address addri = iaddr.GetLocal (); 
        std::cout<<"   "<<addri<<"\n";
  }*/
}
     
static void CalculateThroughput () // in Mbps calculated every 0.5s
{
     std::cout << "  Time: " << Simulator::Now ().GetSeconds () << std::endl;
     Ptr<Ipv4FlowClassifier> classifier=DynamicCast<Ipv4FlowClassifier>(flowMonHelper.GetClassifier());
     std::string proto;
     std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
     

     for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin() ;flow!=stats.end(); flow++)
         {
            Ipv4FlowClassifier::FiveTuple  t = classifier->FindFlow(flow->first);
            std::cout<<t.protocol;
             switch(t.protocol)
             {
             case(6):
                 proto = "TCP";
                 break;
             case(17):
                 proto = "UDP";
                 break;
             default:
                 proto = "";
             }

             std::cout<< "FlowID: " << flow->first << " (" << proto << " "
             << t.sourceAddress << " / " << t.sourcePort << " --> "
             << t.destinationAddress << " / " << t.destinationPort << ")" << "TxByte:"<<flow->second.txBytes<<"  RxBytes:"<<flow->second.rxBytes<<std::endl;
          }
          
          double PktLostRatioSum = 0;
          double ThroughputSum = 0;
          double DelaySum = 0;
          double JitterSum = 0;

        for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin() ;flow!=stats.end(); flow++)
         {
            flowCount =  flow->first;
            TxByteSum += flow->second.txBytes;
            RxByteSum += flow->second.rxBytes;
            TxPacketsSum += flow->second.txPackets;
            RxPacketsSum += flow->second.rxPackets;
            LostPackets += flow->second.lostPackets;
            PktLostRatioSum += ((double)flow->second.txPackets-(double)flow->second.rxPackets)/(double)flow->second.txPackets;

            ThroughputSum  += ((((double)flow->second.rxBytes*8)/(1000000) ) / .5);
            if(flow->second.rxPackets !=0)
            {
              DelaySum += ((double)flow->second.delaySum.GetSeconds())/((double)flow->second.rxPackets);
              JitterSum += ((double)flow->second.jitterSum.GetSeconds())/((double)flow->second.rxPackets);
           }
         }

        PktLostRatio = PktLostRatioSum / flowCount;
        ThroughputAverage = ThroughputSum / flowCount;
        DelayAverage = DelaySum / flowCount;
            std::cout<<"DelayAverage: "<<DelayAverage<<"\n";
            std::cout<<"DelaySum: "<<DelaySum<<"\n";

        JitterAverage = JitterSum / flowCount;
        
    }

std::string GetDate()
{

  /*time_t t= time(NULL);
  struct tm;
  tm = *localtime(&t);
  
  std::string str;  
  str<<tm.tm_mday<<"_"<<tm.tm_mon + 1<<"_"<<tm.tm_year + 1900;*/
std::string str;  
  return str;

}

void exportResults()
{
  std::string pathOut3 = pathOut;
        pathOut3.append(tag.str ());
        pathOut3.append(".csv");
        char *fileName = (char*)pathOut3.c_str();    
          std::ofstream out( fileName );

        if( !out )
        {
           std::cout << "Couldn't open file."  << std::endl;
        }
        else
        {
             out<<"filename,Distance,NumberUes,SimulationTime,IsComp,IsFlat,IsAllot,FlowID,Tx Bytes,Rx Bytes,Tx Packets,Rx Packets," 
             <<"Lost Packets,"<<"Pkt Lost Ratio,"<<"Throughput,"<<"Mean{Delay},"<<"Mean{Jitter},Base1App,Base2App,Base3App,Base4App,"<<
             "Base1Rate,Base2Rate,Base3UERate,Base4Rate\n";
            out<<tag.str()<<",";
            out<<enbDist<<",";
            out<<numUes<<",";
            out<<sec<<",";
            out<<isComp<<",";
            out<<isFlat<<",";
            out<<isAllot<<",";
            out<<flowCount<<",";
            out<<TxByteSum<<",";
            out<<RxByteSum<<",";
            out<<TxPacketsSum<<",";
            out<<RxPacketsSum<<",";
            out<<LostPackets<<",";
            out<<PktLostRatio<<",";
            out<<ThroughputAverage<<",";
            out<<DelayAverage<<",";
            out<<JitterAverage<<",";
            out<<nodeBApplication[0] -1<<",";
            out<<nodeBApplication[1] -1<<",";
            out<<nodeBApplication[2] -1 <<",";
            out<<nodeBApplication[3] -1;
            out<<"\n";
             out.close();
             std::cout<<"Distance,NumberUes,SimulationTime,IsComp,IsFlat,IsAllot,FlowID,Tx Bytes,Rx Bytes,Tx Packets,Rx Packets," 
             <<"Lost Packets,"<<"Pkt Lost Ratio,"<<"Throughput,"<<"Mean{Delay},"<<"Mean{Jitter},Base1App,Base2App,Base3App,Base4App"<<
             "Base1UE,Base2UE,Base3UE,Base4UE\n";
            std::cout<<enbDist<<",";
            std::cout<<numUes<<",";
            std::cout<<sec<<",";
            std::cout<<isComp<<",";
            std::cout<<isFlat<<",";
            std::cout<<isAllot<<",";
            std::cout<<flowCount<<",";
            std::cout<<TxByteSum<<",";
            std::cout<<RxByteSum<<",";
            std::cout<<TxPacketsSum<<",";
            std::cout<<RxPacketsSum<<",";
            std::cout<<LostPackets<<",";
            std::cout<<PktLostRatio<<",";
            std::cout<<ThroughputAverage<<",";
            std::cout<<DelayAverage<<",";
            std::cout<<JitterAverage<<",";
            std::cout<<nodeBApplication[0]<<",";
            std::cout<<nodeBApplication[1]<<",";
            std::cout<<nodeBApplication[2]<<",";
            std::cout<<nodeBApplication[3]<<",";
            std::cout<<bitrate[0]<<",";
            std::cout<<bitrate[1]<<",";
            std::cout<<bitrate[2]<<",";
            std::cout<<bitrate[3];
            
            std::cout<<"\n";

      } 
  
}

/**
 * This simulation script creates two eNodeBs and drops randomly several UEs in
 * a disc around them (same number on both). The number of UEs , the radius of
 * that disc and the distance between the eNodeBs can be configured.
 */
int main (int argc, char *argv[])
{

   pathOut = "/media/sf_Lte/02_28_2013/";
  int isLog = 0;
  CommandLine cmd;
  cmd.AddValue ("enbDist", "distance between the two eNBs", enbDist);
  cmd.AddValue ("radius", "the radius of the disc where UEs are placed around an eNB", radius);
  cmd.AddValue ("numUes", "how many UEs are attached to each eNB", numUes);
  cmd.AddValue ("isComp", "is it a comp scenario ", isComp );
  cmd.AddValue ("sec", "seconds of simualtion ", sec );
  cmd.AddValue ("isLog", "choose if you want logs", isLog );
  cmd.AddValue ("isFlat", "choose wich kind of dispertion you want in place 1= flat , 0 = on one nodeB ", isFlat );
  cmd.AddValue ("isAllot", "choose wich kind of dispertion you want in Application 1= Allot , 0 = on one nodeB ", isAllot );
  cmd.AddValue ("pathValue", "path where to save csv files", pathValue );
  cmd.Parse (argc, argv);
  IntegerValue runValue;
  GlobalValue::GetValueByName ("RngRun", runValue);

  cmd.Parse (argc, argv);

  //enable logs
  if(isLog == 1)
   { 
    //LogComponentEnable("LteEnbPhy", LogLevel(LOG_LEVEL_INFO | LOG_PREFIX_TIME| LOG_FUNCTION ));
    //LogComponentEnable("FdMtFfMacScheduler", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME| LOG_FUNCTION ));
    //LogComponentEnable ("LteStatsCalculator", LOG_LEVEL_INFO);
    //LogComponentEnable ("LteInterference" , LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME));
    LogComponentEnable ("UdpEchoClientHelper" , LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME| LOG_FUNCTION ));
  }

  // determine the string tag that identifies this simulation run
  tag  << "_enbDist" << std::setw (3) << std::setfill ('0') << std::fixed << std::setprecision (0) << enbDist
       << "_numUes"  << std::setw (3) << std::setfill ('0')  << numUes
       << "_isComp"  << std::setw (3) << std::setfill ('0')  << isComp
       << "_sec"  << std::setw (3) << std::setfill ('0')  << sec
       << "_Flat"  << std::setw (3) << std::setfill ('0')  << isFlat
       << "_Allot"  << std::setw (3) << std::setfill ('0')  << isAllot
       << GetDate();

sec = sec * 100;

std::string dlOutFname = "DlRlcStats";
  dlOutFname.append (tag.str ());

  std::string ulOutFname = "UlRlcStats";
  ulOutFname.append (tag.str ());


  std::string outputpath = pathOut; //"Results" ;
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlRlcOutputFilename", StringValue (outputpath+"/DlRlcStats" + tag.str() + ".txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlRlcOutputFilename", StringValue (outputpath+"/UlRlcStats"+ tag.str() +".txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename", StringValue (outputpath+"/DlPdcpStats"+ tag.str() +".txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename", StringValue (outputpath+"/UlPdcpStats"+ tag.str() +".txt"));
  Config::SetDefault ("ns3::MacStatsCalculator::DlOutputFilename", StringValue (outputpath+"/DlMacStats"+ tag.str() +".txt"));
  Config::SetDefault ("ns3::MacStatsCalculator::UlOutputFilename", StringValue (outputpath+"/UlMacStats"+ tag.str() +".txt"));
std::cout<<outputpath+"/Dlmacstats.txt";
  std::cout<<"File name: "<<"\n";
  std::cout<<tag.str()<<"\n";
  std::cout<<"IsFlat: "<< isFlat<<"\n";
  std::cout<<"IsAllot: "<< isAllot<<"\n";

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<EpcHelper>  epcHelper = CreateObject<EpcHelper> ();
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
  lteHelper->SetEpcHelper (epcHelper);
 
  // Create Nodes: eNodeB and UE
  int UesNumber = 4; 

  NodeContainer enbNodes;
  NodeContainer ueNodes[4];
  enbNodes.Create (4);

  int numberByApplication[4];

  if(isAllot == 0 )
  {
    std::cout<<"Not allot every Aplication have "<<numUes<<" user.\n";
    for( int v = 0; v< UesNumber ; v++)
    {
      numberByApplication[v] = numUes;
    }  
  }
  else
  {
    int sum = numUes * 4; 
    numberByApplication[0] = trunc(sum * 0.45); 
    numberByApplication[1] = trunc(sum * 0.20); 
    numberByApplication[2] = trunc(sum * 0.20); 
    numberByApplication[3] = trunc(sum * 0.15); 
  }

  for(int i = 0; i<4; i++)
  {
    std::cout<<"\nApplication number: "<<i<<"  "<<numberByApplication[i];
  }

  for( int v = 0; v< UesNumber ; v++)
  {
    ueNodes[v].Create (numberByApplication[v]); 
  }
  
  // Position of eNBs
  double max;
  double delta;
  max = enbDist * 2;
  delta = enbDist / 2;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (delta, delta, 0.0));
  positionAlloc->Add (Vector (max - delta, delta, 0.0));
  positionAlloc->Add (Vector (delta, max - delta, 0.0));
  positionAlloc->Add (Vector (max - delta, max - delta, 0.0));

  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (positionAlloc);
  enbMobility.Install (enbNodes);

  // Position of UEs attached to eNB 1

  int divisor = 1;

  if(isFlat == 0)
  {
    divisor = 2;
  }
  
  for(int z = 0; z< UesNumber; z++)
  {
    MobilityHelper uemobility;
    uemobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                      "X", DoubleValue (enbDist / divisor),
                                      "Y", DoubleValue (enbDist /divisor),
                                      "rho", DoubleValue (enbDist / divisor));
    uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    uemobility.Install (ueNodes[z]);
  }
  
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);
  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);


  std::cout<<"\npgw: "<<GetIp(pgw)<<"\n";
  std::cout<<"remoteHost: "<<GetIp(remoteHost)<<"\n";
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs[UesNumber];
  
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
 
  for( int z = 0 ; z< UesNumber; z++)
  {
    ueDevs[z] = lteHelper->InstallUeDevice (ueNodes[z]);
  }
  
  // Attach UEs to a eNB
  if (isComp==1)
  {
    for( int z = 0 ; z< UesNumber; z++)
    {
	   lteHelper->Attach (ueDevs[z], enbDevs.Get (z));
    }
	 
  }
  else
  {
    for( int z = 0; z < UesNumber; z++)
    {
      for (int i = 0; i < numberByApplication[z]; i++)
      {
		    lteHelper->AttachToClosestEnb(ueDevs[z].Get(i), enbDevs);
		  }
	  }
  }

  //Install the IP stack on the UEs
  for( int z = 0 ; z< UesNumber; z++)
  {
    internet.Install (ueNodes[z]);
  }


  Ipv4InterfaceContainer ueIpIface[UesNumber];

  for( int z = 0 ; z< UesNumber; z++)
  {
    ueIpIface[z] = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs[z]));
  }

  // Assign IP address to UEs, and install applications
    for( int z = 0 ; z< UesNumber; z++)
  {
    for (int u = 0; u < numberByApplication[z]; u++)
    {
      Ptr<Node> ueNode = ueNodes[z].Get (u);
      
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
  }
   
  // Activate an EPS bearer on all UEs
  enum EpsBearer::Qci q1 = EpsBearer::GBR_CONV_VOICE; 
  enum EpsBearer::Qci q2 = EpsBearer::GBR_CONV_VOICE;
  enum EpsBearer::Qci q3 = EpsBearer::GBR_CONV_VOICE;
  enum EpsBearer::Qci q4 = EpsBearer::GBR_CONV_VOICE;

  EpsBearer bearer0(q1);
  EpsBearer bearer1(q2);
  EpsBearer bearer2(q3);
  EpsBearer bearer3(q4);
    
  lteHelper->ActivateEpsBearer (ueDevs[0], bearer0, EpcTft::Default ());
	lteHelper->ActivateEpsBearer (ueDevs[1], bearer1, EpcTft::Default ());
  lteHelper->ActivateEpsBearer (ueDevs[2], bearer2, EpcTft::Default ());
	lteHelper->ActivateEpsBearer (ueDevs[3], bearer3, EpcTft::Default ());


uint16_t RHPort = 94; //Port used for Google Talk voice/video -
//uint16_t DataPort =  80;       


int VoipBitrate = 12;
int VideoBitrate = 668;
int TvBitrate = 11588;
int WebBitrate = 1;


bitrate[0] = VoipBitrate;
bitrate[1] = VideoBitrate;
bitrate[2] = TvBitrate;
bitrate[3] = WebBitrate;
        ApplicationContainer onoffAppsRH;
        ApplicationContainer onoffAppsUE;
        std::cout<<"\n";
/*for(int w= 0 ; w < 4 ; w++)
{  
  std::string nodeBCandstr = GetIp(enbNodes.Get(w));*/
  for( int z = 0 ; z< UesNumber; z++)
  {int w = z;
        for( int v = 0 ; v < numberByApplication[z] ; v++)
    {
      
      Ptr<Node> ueNode =  ueDevs[z].Get (v)->GetNode();
      Ptr<Node> endNode =  ueNode->GetDevice(0)->GetObject<LteUeNetDevice>()->GetTargetEnb()->GetNode();

      // ServerIP
      Ptr<Ipv4> ipv4 = endNode->GetObject<Ipv4>();
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
      Ipv4Address addressServer = iaddr.GetLocal ();
      std::string nodeBcurr = GetIp(endNode);
  
   /*   if(nodeBcurr.compare(nodeBCandstr) == 0)
      {*/

        // ClientIP
        Ptr<Ipv4> ipv41 = ueNode -> GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr2 = ipv41->GetAddress (1,0);
        Ipv4Address addriClient = iaddr2.GetLocal ();
      
        InetSocketAddress UE = InetSocketAddress (addressServer, ++RHPort); //UE

         UdpEchoClientHelper ClientHTTP1 (addressServer, RHPort);
         ClientHTTP1.SetAttribute ("MaxPackets", UintegerValue (1000));
         ClientHTTP1.SetAttribute ("Interval", TimeValue (Seconds (0.01)));
         ClientHTTP1.SetAttribute ("PacketSize", UintegerValue (bitrate[z] * 100/ numUes));
         ApplicationContainer clientAppsHTTP1 = ClientHTTP1.Install (ueNode);
      //   clientAppsHTTP1.Start (Seconds (0.01));

        // Server Application
      /*  OnOffHelper onoffRH ("ns3::UdpSocketFactory", Address (UE));
        onoffRH.SetAttribute ("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1000]")); 
        onoffRH.SetAttribute ("OffTime",  StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoffRH.SetAttribute ("DataRate", DataRateValue (DataRate(bitrate[z])));
        //onoffRH.SetAttribute ("PacketSize", UintegerValue (7));
        ApplicationContainer apps = onoffRH.Install (endNode); //sink.Install (endNode);
*/
        // Start in right fourth of time
        int startTime = sec * w;
        int stopTime = sec * (w +1);
        clientAppsHTTP1.Start (MilliSeconds (startTime));
        clientAppsHTTP1.Stop (MilliSeconds (stopTime));
        std::cout<<addriClient<<"   "<<addressServer<<": start at: "<<startTime<<" stop at: "<<stopTime<<"\n";   

        UdpEchoServerHelper ueServerVIDEO (RHPort);      
        ApplicationContainer serverAppsVIDEO1 = ueServerVIDEO.Install (endNode);
        // UE application Sink
        PacketSinkHelper sink2 ("ns3::UdpSocketFactory", Address(UE));
        onoffAppsUE.Add (sink2.Install (ueNode));
      //}
    }
  }
//}
      onoffAppsRH.Start (Seconds (0));
      onoffAppsUE.Start (Seconds (0));
ExportGnuFile(enbNodes, ueDevs);
   
  std::cout<<"\nNodeB"<<"     "<<"Number of applications\n";
  for(uint i = 0 ; i <4 ; i++)
  {
    nodeBApplication[i] = enbNodes.Get(i)->GetNApplications(); 
    std::cout<<i<<"  "<<nodeBApplication[i] -1<<"\n";
  }


  
  
std::cout<<"\n\nNodeBs Ip and place\n";
  
    PrintNodePlace(enbNodes,4);
  
std::cout<<"\n\nUes Ip and place\n";

  for( int z = 0 ; z< UesNumber; z++)
  {
    PrintNodePlace(ueNodes[z],numberByApplication[z]);
  }
  
  
  std::cout<<"Simulation\n";
  std::cout<<"Number of Base Stations: 4'\n";
  std::cout<<"Distance Between every Base Station: "<<enbDist<<"\n";
  std::cout<<"Number of Ues by Base Station: "<<numUes<<"\n";
  std::cout<<"Simulation length: "<<sec<<"\n";
  std::cout<<"Comp: "<<isComp<<"\n";

  // Tracing
  flowMonHelper.SetMonitorAttribute("StartTime", TimeValue(Time(1.0)));
  flowMonHelper.SetMonitorAttribute("JitterBinWidth", ns3::DoubleValue(0.001));
  flowMonHelper.SetMonitorAttribute("DelayBinWidth", ns3::DoubleValue(0.001));
  flowMonHelper.SetMonitorAttribute("PacketSizeBinWidth", DoubleValue(20));
  flowmon = flowMonHelper.InstallAll(); 
  


//lteHelper->EnablePhyTraces ();
lteHelper->EnableMacTraces ();
lteHelper->EnableRlcTraces ();
lteHelper->EnablePdcpTraces ();

/*   lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();*/

  Simulator::Stop(MilliSeconds(sec * 4));

  std::string pathOut4 = pathOut;
  pathOut4.append(tag.str ());
  pathOut4.append(".tr");
  char *fileName = (char*)pathOut4.c_str();    
  AsciiTraceHelper ascii;


  p2ph.EnableAsciiAll (ascii.CreateFileStream (fileName));
  Simulator::Run ();
  CalculateThroughput();
  exportResults();
  Simulator::Destroy ();
  std::cout<<"Simulation finish "<<numUes<<"\n";

  return 0;
}