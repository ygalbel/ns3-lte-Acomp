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
int VoipBitrate = 85;
int VideoBitrate = 500;
int FiLesharingBitRate = 1000;
int WebBitrate = 1000;
int numberByApplication[4];
int DelayByGroup[4];  
int RxByGroup[4];
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
int part = 0;
int nodeBApplication[4];
char *pathValue;
std::string pathOut;
int bitrate[4];
Vector NodeBVector[4];
//int HxbyGroup[4];

int GetIndexOfClosestEnb (Ptr<NetDevice> ueDevice)
 {
       
   Vector uepos = ueDevice->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
   double minDistance = std::numeric_limits<double>::infinity ();
   int index = 0;
   for (int i = 0 ; i<4 ; i++)
     {
       Vector enbpos =  NodeBVector[i];
       double distance = CalculateDistance (uepos, enbpos);
       if (distance < minDistance)
         {
           minDistance = distance;
          index = i; 
         }      
     }
  // std::cout<<uepos.x<<" "<<uepos.y<<" base: "<<index<<"\n";
   return index;
 }

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

int GetInterval(int group)
{
  /*bitrate[0] =  FiLesharingBitRate;
  bitrate[1] =  VideoBitrate;
  bitrate[2] =  VoipBitrate ;
  bitrate[3] =  WebBitrate;
*/
  if(group == 0) // File Sharing 1000 * 15
  {
    return 66;
  }
  if(group == 1) // Video 500 * 15 = 7.5
  {
    return 66;
  }
  if(group == 2)
  {
    return 200;
  }
  if(group == 3)
  {
    return 100;
  }

  return 10;
}
std::string GetBitRate()
{
  int RatePerUser = 3;
  int summer = 0 ;
  if(isComp == 0)
  {
    summer = RatePerUser * numUes;
  } 
  else
  {
    int sumRate = bitrate[0] * numberByApplication[0] +
                  bitrate[1] * numberByApplication[1] +
                  bitrate[2] * numberByApplication[2] +
                  bitrate[3] * numberByApplication[3] ;

    std::cout<<"\nNumber: "<<sumRate;
    summer = trunc(4 * RatePerUser * numUes * (bitrate[part] *numberByApplication[part])/ sumRate);
  }
        std::ostringstream result;
        result << summer<<"Mb/s";
        std::cout<<"\n"<<result.str()<<"    TTTTTTTTTTTTTTTTTTTTT\n";
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
        std::cout<<GetIp(node.Get(i))<<"\t"<<GetNodePlace(node.Get(i))<<"\n";
		  }
}

void ExportGnuFile(NodeContainer ueNode)
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

          for(int i = 0 ; i < 4 ; i++)
          {
            out<<"# Base"<<i<<"\n";
            out<<"\t"<<NodeBVector[i].x<<"\t"<<NodeBVector[i].y<<"\n";
            out<<"\n\n";
          }
          for(int z= 0 ; z < 4; z++)
          {
            if(z != part)
            {
              out<<"\t"<<NodeBVector[z].x<<"\t"<<NodeBVector[z].y<<"\n";
            }
            else
            for(uint i=0; i<ueNode.GetN(); i++)
            {                    
               out<<GetNodePlace(ueNode.Get (i));
            }
              out<<"\n\n";
        }
        }
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
             << t.sourceAddress << " /" << t.sourcePort << " --> "
             << t.destinationAddress << " /" << t.destinationPort << ")" << "TxByte:"<<flow->second.txBytes<<"  RxBytes:"
             <<flow->second.rxBytes<<std::endl;

           /* std::cout<<"PacketDropperSize: "<<flow->second.bytesDropped.size()<<"\n";
            std::cout<<"LostPackets: "<<flow->second.lostPackets<<"\n";
            for(uint32_t  i =0 ; i < flow->second.bytesDropped.size(); i++)
              {
              std::cout<<"\nPacket Dropped reason 0"<<i<<": "<<flow->second.bytesDropped.at(i);
            }*/
          }
          
          double PktLostRatioSum = 0;
          double ThroughputSum = 0;
          double DelaySum = 0;
          double JitterSum = 0;
int groupcurr;
 
  for(int i = 0 ; i < 4 ; i++)
  {
DelayByGroup[i] = 0;
//HxbyGroup[i] = 0;
RxByGroup[i] = 0;
  }
          for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin() ;flow!=stats.end(); flow++)
         {
          Ipv4FlowClassifier::FiveTuple  t = classifier->FindFlow(flow->first);
           if(t.sourcePort != 2152 && t.destinationPort != 2152)
           {
            groupcurr = t.destinationPort / 100;
            std::cout<<"#####"<<groupcurr<<" ";
              flowCount =  flow->first;
              TxByteSum += flow->second.txBytes;
              RxByteSum += flow->second.rxBytes;
              TxPacketsSum += flow->second.txPackets;
              RxPacketsSum += flow->second.rxPackets;
              LostPackets += flow->second.lostPackets;
              DelayByGroup[groupcurr] += TxByteSum;
              RxByGroup[groupcurr] += RxByteSum;
              
              PktLostRatioSum += ((double)flow->second.txPackets-(double)flow->second.rxPackets)/(double)flow->second.txPackets;
  
              ThroughputSum  += ((((double)flow->second.rxBytes*8)) / .5);
              if(flow->second.rxPackets !=0)
              {
                //DelayByGroup[groupcurr] += ((double)flow->second.delaySum.GetSeconds())/((double)flow->second.rxPackets);


                DelaySum += ((double)flow->second.delaySum.GetSeconds())/((double)flow->second.rxPackets);
                JitterSum += ((double)flow->second.jitterSum.GetSeconds())/((double)flow->second.rxPackets);
              }
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
             out<<"part,filename,Distance,NumberUes,SimulationTime,IsComp,IsFlat,IsAllot,FlowID,Tx Bytes,Rx Bytes" 
             <<"Lost Packets,"<<"Pkt Lost Ratio,"<<"Throughput,"<<"Delay,"<<"Jitter"
             <<"RxBytes0,TxByte0,"<<
             "RxBytes1,TxByte1,"<<
             "RxBytes2,TxByte2,"<<
             "RxBytes3,TxByte3,BitRate\n";
             out<<part<<",";
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
            out<<LostPackets<<",";
            out<<PktLostRatio<<",";
            out<<ThroughputAverage<<",";
            out<<DelayAverage<<",";
            out<<JitterAverage<<",";
            out<<RxByGroup[0]<<",";
           // out<<HxByGroup[0]<<",";
            out<<DelayByGroup[0]<<",";
            out<<RxByGroup[1]<<",";
           // out<<HxByGroup[1]<<",";
            out<<DelayByGroup[1]<<",";
            out<<RxByGroup[2]<<",";
         //   out<<HxByGroup[2]<<",";
            out<<DelayByGroup[2]<<",";
            out<<RxByGroup[3]<<",";
          //  out<<HxByGroup[3]<<",";
            out<<DelayByGroup[3]<<",";
            out<<GetBitRate();
            out<<"\n";
             out.close();
             std::cout<<"part,Distance,NumberUes,SimulationTime,IsComp,IsFlat,IsAllot,FlowID,Tx Bytes,Rx Bytes,Tx Packets,Rx Packets," 
             <<"Lost Packets,"<<"Pkt Lost Ratio,"<<"Throughput,"<<"Mean{Delay},"<<"Mean{Jitter},Base1App,Base2App,Base3App,Base4App"<<
             "Base1UE,Base2UE,Base3UE,Base4UE\n";
             std::cout<<part<<",";
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
   pathOut = "/media/sf_Lte/04_06_2013/";
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
  cmd.AddValue ("part", "Part to run", part );
  cmd.Parse (argc, argv);
  IntegerValue runValue;
  GlobalValue::GetValueByName ("RngRun", runValue);

  cmd.Parse (argc, argv);

  //enable logs
  if(isLog == 1)
   { 
    LogComponentEnable("DataRate", LogLevel(LOG_LEVEL_INFO | LOG_PREFIX_TIME| LOG_FUNCTION ));
   // LogComponentEnable("FlowMonitor", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME| LOG_FUNCTION ));
    //LogComponentEnable ("LteStatsCalculator", LOG_LEVEL_INFO);
    //LogComponentEnable ("LteInterference" , LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME));
    //LogComponentEnable ("EpcEnbApplication" , LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_TIME| LOG_FUNCTION ));
  }

  // determine the string tag that identifies this simulation run
  tag  << "_enbDist" << std::setw (3) << std::setfill ('0') << std::fixed << std::setprecision (0) << enbDist
       << "_numUes"  << std::setw (3) << std::setfill ('0')  << numUes
       << "_isComp"  << std::setw (3) << std::setfill ('0')  << isComp
       << "_sec"  << std::setw (3) << std::setfill ('0')  << sec
       << "_Flat"  << std::setw (3) << std::setfill ('0')  << isFlat
       << "_Allot"  << std::setw (3) << std::setfill ('0')  << isAllot       
       << "_Part"  << std::setw (3) << std::setfill ('0')  << part
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
  std::cout<<"\nPart: "<<part;
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<EpcHelper>  epcHelper = CreateObject<EpcHelper> ();
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
  lteHelper->SetSchedulerType ("ns3::FdTbfqFfMacScheduler");  // FD-TBFQ scheduler
  lteHelper->SetEpcHelper (epcHelper);
 
  // Create Nodes: eNodeB and UE
  int UesNumber = 4; 
  int group[numUes * 4];
  NodeContainer enbNodes;
  NodeContainer ueNodes[4];
  enbNodes.Create (1);

  

  if(isAllot == 0 )
  {
    for( int v = 0; v< UesNumber ; v++)
    {
      numberByApplication[v] = numUes;
    }  
  }
  else
  {
    int currentR = 0; 
    int sum = numUes * 4; 
    numberByApplication[0] = trunc(sum * 0.26);  // Sharing 
    currentR +=  trunc(sum * 0.26);
    numberByApplication[1] = trunc(sum * 0.42);  // Video 
    currentR +=  trunc(sum * 0.42);
    numberByApplication[2] = trunc(sum * 0.05);  // Voice
    currentR +=  trunc(sum * 0.05);
    numberByApplication[3] = trunc(sum * 0.27); // Browsing
    currentR +=  trunc(sum * 0.27);

    numberByApplication[1] += sum - currentR;
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
  NodeBVector[0] = Vector (delta, delta, 0.0);
  NodeBVector[1] = Vector (max - delta, delta, 0.0);
  NodeBVector[2] = Vector (delta, max - delta, 0.0);
  NodeBVector[3] = Vector (max - delta, max - delta, 0.0)   ;

  positionAlloc->Add (NodeBVector[part]);

  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (positionAlloc);
  enbMobility.Install (enbNodes);

  // Position of UEs attached to eNB 1

  int divisor = 1;
  int alpha = 0;
  if(isFlat == 0)
  {
    divisor = 2;
  }
  else if( isFlat == 2)
  {
    alpha =  3 * enbDist / 4;
    divisor = 4;
  }
  else if( isFlat == 3)
  {
    alpha =  2 * enbDist / 4;
    divisor = 4;
  }

  
  for(int z = 0; z< UesNumber; z++)
  {
    MobilityHelper uemobility;
    uemobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                      "X", DoubleValue (enbDist / divisor  + alpha),
                                      "Y", DoubleValue (enbDist /divisor + alpha),
                                      "rho", DoubleValue (enbDist / divisor ));
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

  
  bitrate[0] =  FiLesharingBitRate;
  bitrate[1] =  VideoBitrate;
  bitrate[2] =  VoipBitrate ;
  bitrate[3] =  WebBitrate;

  PointToPointHelper p2ph;
  

  
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (GetBitRate())));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);


  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs[4];
  NetDeviceContainer current;
  NodeContainer currentNode;
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  
  for(int z = 0 ; z < 4 ; z++)
  {
    ueDevs[z] = lteHelper->InstallUeDevice (ueNodes[z]);
  }
  
  // Attach UEs to a eNB
  if (isComp==1)
  {

    int z = part;
      for (int i = 0; i < numberByApplication[z]; i++)
      {
	      lteHelper->Attach (ueDevs[z].Get(i), enbDevs.Get (0));
        current.Add(ueDevs[z].Get(i));
        currentNode.Add(ueDevs[z].Get(i)->GetNode());
        group[currentNode.GetN() - 1] = part;
      }
  }
  else
  {
    for( int z = 0; z < UesNumber; z++)
    {
      for (int i = 0; i < numberByApplication[z]; i++)
      {
        if(GetIndexOfClosestEnb(ueDevs[z].Get(i)) == part)
        {
		      lteHelper->AttachToClosestEnb(ueDevs[z].Get(i), enbDevs);
          current.Add(ueDevs[z].Get(i));
          currentNode.Add(ueDevs[z].Get(i)->GetNode());
          group[currentNode.GetN() - 1] = z;
        }
		  }
	  }
  }

  std::cout<<"Connected: "<<current.GetN();

  //Install the IP stack on the UEs
  
  internet.Install (currentNode);

  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (current));
  

  // Assign IP address to UEs, and install applications
    
    for (uint u = 0; u < current.GetN(); u++)
    {
      std::cout<<"Part:"<<u<<" "<<numberByApplication[part]<<"\n";
      Ptr<Node> ueNode = current.Get(u)->GetNode ();
      
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
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
    
    //TODO: change to PART!!!!
  lteHelper->ActivateEpsBearer (current, bearer0, EpcTft::Default ());
	





int port[4];
port[0] = 10;
port[1] = 101;
port[2] = 201;
port[3] = 301;
//int dlPort = 100;
//int ulPort = 200;
//float ratio = 10;
        std::cout<<"\n";
        int interPacketInterval = 1;
        //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  //int z= part;      
  ApplicationContainer clientAppsHTTP1 ;
        // well-known echo port number
      //uint16_t ulPort = 99;  // well-known echo port number
std::cout<<group[0];
ApplicationContainer sourceApps;
  for( uint v = 0 ; v < current.GetN() ; v++)
    {
      Ptr<Node> ueNode =  current.Get (v)->GetNode();
      Ptr<Node> endNode = enbNodes.Get(0);// ueNode->GetDevice(0)->GetObject<LteUeNetDevice>()->GetTargetEnb()->GetNode();
      /*std::cout<<v<<" ";
      std::cout<<group[v]<<" ";
      std::cout<<bitrate[group[v]]<<"\n"*/
      //->GetObject<LteUeNetDevice>()->GetTargetEnb()
      // ServerIP
      Ptr<Ipv4> ipv4 = ueNode->GetObject<Ipv4>();
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
      Ipv4Address addressUe = iaddr.GetLocal ();

//UDP WORK!!!
             interPacketInterval = GetInterval(group[v]);
               UdpClientHelper dlClientHelper (addressUe, ++port[group[v]]);
               dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
               dlClientHelper.SetAttribute ("MaxPackets", UintegerValue(1200));
               dlClientHelper.SetAttribute("PacketSize", UintegerValue(bitrate[group[v]]));
               sourceApps.Add (dlClientHelper.Install (remoteHost));

               PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", 
                                                    InetSocketAddress (Ipv4Address::GetAny (), port[group[v]]));
              sourceApps.Add (dlPacketSinkHelper.Install (ueNode));
             
                 
           /*    NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
               UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
               clientApps.Add (ulClientHelper.Install (ue));
               PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", 
                                                    InetSocketAddress (Ipv4Address::GetAny (), ulPort));
               serverApps.Add (ulPacketSinkHelper.Install (remoteHost));  */
             

                 //Tcp Work!!
                    /*  BulkSendHelper dl2ClientHelper ("ns3::TcpSocketFactory",
                                                     InetSocketAddress (addressUe, ++dlPort));
                      dl2ClientHelper.SetAttribute ("MaxBytes", UintegerValue (1200));
                      sourceApps.Add (dl2ClientHelper.Install (remoteHost));
                      PacketSinkHelper dl2PacketSinkHelper ("ns3::TcpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                      sourceApps.Add (dl2PacketSinkHelper.Install (ueNode));*/

                 
                  /*    BulkSendHelper ulClientHelper ("ns3::TcpSocketFactory",
                                                     InetSocketAddress (remoteHostAddr, ulPort));
                      ulClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));                  
                      sourceApps.Add (ulClientHelper.Install (ueNode));
                      PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                      sourceApps.Add (ulPacketSinkHelper.Install (remoteHost));*/

/*      BulkSendHelper source ("ns3::UdpSocketFactory",
                             InetSocketAddress (addressUe, port));
      // Set the amount of data to send in bytes.  Zero is unlimited.
      source.SetAttribute ("MaxBytes", UintegerValue (0));
      ApplicationContainer sourceApps = source.Install (endNode);*/

      //
      // Create a PacketSinkApplication and install it on node 1
      //
      /*PacketSinkHelper sink ("ns3::TcpSocketFactory",
                             InetSocketAddress (Ipv4Address::GetAny (), port));
       sinkApps = sink.Install (ueNode);
      sinkApps.Start (Seconds (0.0));
      sinkApps.Stop (Seconds (10.0));
        */
        /*UdpClientHelper dlClient (addressClient, dlPort++);
        dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));*/
        //dlClient.SetAttribute("PacketSize", UintegerValue(bitrate[group[v]]));


        /*UdpClientHelper ulClient (addressClient, ulPort++);
        ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
        ulClient.SetAttribute("PacketSize", UintegerValue(bitrate[group[v]]));*/

  /*      UdpClientHelper client (remoteHostAddr, RHPort++);
        client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        client.SetAttribute ("MaxPackets", UintegerValue(1000000));
*/
        //clientAppsHTTP1.Add (dlClient.Install (remoteHost));
        /*clientAppsHTTP1.Add (ulClient.Install (ueNode));
    
        int startTime = 0;
        int stopTime = sec;
        clientAppsHTTP1.Start (MilliSeconds (startTime + v));
        clientAppsHTTP1.Stop (MilliSeconds (stopTime));*/
    }
      sourceApps.Start (Seconds (0.0));
      sourceApps.Stop (Seconds (10.0));
 
ExportGnuFile(currentNode);
   
  /*std::cout<<"\nNodeB"<<"     "<<"Number of applications\n";
  for(uint i = 0 ; i <4 ; i++)
  {
    nodeBApplication[i] = enbNodes.Get(i)->GetNApplications(); 
    std::cout<<i<<"  "<<nodeBApplication[i] -1<<"\n";
  }*/


  
  
/*std::cout<<"\n\nNodeBs Ip and place\n";
  
    PrintNodePlace(enbNodes,4);
  
std::cout<<"\n\nUes Ip and place\n";

  for( int z = 0 ; z< UesNumber; z++)
  {
    PrintNodePlace(ueNodes[z],numberByApplication[z]);
  }
  */
  
  std::cout<<"Simulation\n";
  std::cout<<"Number of Base Stations: 4'\n";
  std::cout<<"Distance Between every Base Station: "<<enbDist<<"\n";
  std::cout<<"Number of Ues by Base Station: "<<numUes<<"\n";
  std::cout<<"Simulation length: "<<sec<<"\n";
  std::cout<<"Comp: "<<isComp<<"\n";

  // Tracing
  flowMonHelper.SetMonitorAttribute("StartTime", TimeValue(Time(0.0)));
  flowMonHelper.SetMonitorAttribute("JitterBinWidth", ns3::DoubleValue(0.001));
  flowMonHelper.SetMonitorAttribute("DelayBinWidth", ns3::DoubleValue(0.001));
  flowMonHelper.SetMonitorAttribute("PacketSizeBinWidth", DoubleValue(20));
  flowmon = flowMonHelper.Install(currentNode); 
  flowmon = flowMonHelper.Install(remoteHostContainer); 
  


//lteHelper->EnablePhyTraces ();
/*lteHelper->EnableMacTraces ();
lteHelper->EnableRlcTraces ();
lteHelper->EnablePdcpTraces ();*/


  Simulator::Stop(MilliSeconds(sec));

  std::string pathOut4 = pathOut;
  pathOut4.append(tag.str ());
  pathOut4.append(".tr");
  char *fileName = (char*)pathOut4.c_str();    
  AsciiTraceHelper ascii;
  p2ph.EnableAsciiAll (ascii.CreateFileStream (fileName));


  Simulator::Run ();

 /* Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
   std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;*/
  CalculateThroughput();
  exportResults();
  Simulator::Destroy ();
  std::cout<<"Simulation finish "<<numUes<<"\n";

  return 0;
}