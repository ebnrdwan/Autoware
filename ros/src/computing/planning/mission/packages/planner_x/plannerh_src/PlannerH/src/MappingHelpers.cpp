/*
 * MappingHelpers.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: ai-driver
 */

#include "MappingHelpers.h"
#include "MatrixOperations.h"
#include "PlanningHelpers.h"
#include "proj_api.h"
#include "geo_pos_conv.hh"


using namespace UtilityHNS;
using namespace std;

namespace PlannerHNS {



MappingHelpers::MappingHelpers() {
	// TODO Auto-generated constructor stub

}

MappingHelpers::~MappingHelpers() {
	// TODO Auto-generated destructor stub
}

GPSPoint MappingHelpers::GetTransformationOrigin()
{
	return GPSPoint(-3700, 99427, -88,0);
}

void MappingHelpers::ConstructRoadNetworkFromRosMessage(const std::vector<UtilityHNS::AisanLanesFileReader::AisanLane>& lanes_data,
		const std::vector<UtilityHNS::AisanPointsFileReader::AisanPoints>& points_data,
		const std::vector<UtilityHNS::AisanCenterLinesFileReader::AisanCenterLine>& dt_data,
		const GPSPoint& origin, RoadNetwork& map)
{
	vector<Lane> roadLanes;
	Lane lane_obj;
	WayPoint lastPoint;

	int prev_FLID = -1;
	bool bNewLane = true;
	int laneIDSeq = 0;
	for(unsigned int l= 0; l < lanes_data.size(); l++)
	{
		if(lanes_data.at(l).LnID != prev_FLID)
		{
			if(laneIDSeq != 0) //first lane
			{
				lane_obj.toIds.push_back(prev_FLID);
				roadLanes.push_back(lane_obj);
			}
			WayPoint wp;
			prev_FLID = lanes_data.at(l).FLID;
			bNewLane = true;
			laneIDSeq++;
			lane_obj = Lane();
			lane_obj.speed = lanes_data.at(l).LimitVel;
			lane_obj.id = lanes_data.at(l).LnID;
			lane_obj.fromIds.push_back(lanes_data.at(l).BLID);
			lane_obj.roadId = laneIDSeq;
			bool bFound = GetWayPoint(lane_obj.id, lane_obj.id, lanes_data.at(l).RefVel, lanes_data.at(l).DID, dt_data,
					points_data, origin,wp);
			wp.fromIds.push_back(lanes_data.at(l).BLID);
			wp.toIds.push_back(prev_FLID);

			if(bFound)
				lane_obj.points.push_back(wp);

		}
		else
		{
			bNewLane = false;
		}

		if(!bNewLane)
		{

			WayPoint wp;
			bool bFound = GetWayPoint(lanes_data.at(l).LnID, lane_obj.id, lanes_data.at(l).RefVel,lanes_data.at(l).DID, dt_data,
								points_data,origin, wp);


			if(lanes_data.at(l).JCT > 0)
			{
				//Link intersections
				if(lanes_data.at(l).FLID2 > 0)
				{
					lane_obj.toIds.push_back(lanes_data.at(l).FLID2);
					wp.toIds.push_back(lanes_data.at(l).FLID2);
				}
				if(lanes_data.at(l).FLID3 > 0)
				{
					lane_obj.toIds.push_back(lanes_data.at(l).FLID3);
					wp.toIds.push_back(lanes_data.at(l).FLID3);
				}
				if(lanes_data.at(l).FLID4 > 0)
				{
					lane_obj.toIds.push_back(lanes_data.at(l).FLID4);
					wp.toIds.push_back(lanes_data.at(l).FLID4);
				}

				if(lanes_data.at(l).BLID2 > 0)
				{
					lane_obj.fromIds.push_back(lanes_data.at(l).BLID2);
					wp.fromIds.push_back(lanes_data.at(l).BLID2);
				}
				if(lanes_data.at(l).BLID3 > 0)
				{
					lane_obj.fromIds.push_back(lanes_data.at(l).BLID3);
					wp.fromIds.push_back(lanes_data.at(l).BLID3);
				}
				if(lanes_data.at(l).BLID4 > 0)
				{
					lane_obj.fromIds.push_back(lanes_data.at(l).BLID4);
					wp.fromIds.push_back(lanes_data.at(l).BLID4);
				}

			}

			wp.fromIds.push_back(lanes_data.at(l).BLID);
			wp.toIds.push_back(lanes_data.at(l).FLID);



			if(bFound)
			{
				lane_obj.points.push_back(wp);
				lastPoint = wp;
			}

			prev_FLID = lanes_data.at(l).FLID;
		}

//			cout << " ID = " << lanes_data.at(l).LnID << ", BLID: " << lanes_data.at(l).BLID << ", FLID:" << lanes_data.at(l).FLID<<
//					", JCT:" << lanes_data.at(l).JCT << ", LCnt:" << lanes_data.at(l).LCnt << ", LNo:" << lanes_data.at(l).Lno <<
//					", SecID:" << lanes_data.at(l).RoadSecID <<
//					", BLID2: " << lanes_data.at(l).BLID2 << ", FLID2: " << lanes_data.at(l).FLID2 <<
//					", BLID3: " << lanes_data.at(l).BLID3 << ", FLID3: " << lanes_data.at(l).FLID3 <<
//					", BLID4: " << lanes_data.at(l).BLID4 << ", FLID4: " << lanes_data.at(l).FLID4 << endl;
	}

	//delete first two lanes !!!!! Don'e know why
	if(roadLanes.size() > 0)
		roadLanes.erase(roadLanes.begin()+0);
	if(roadLanes.size() > 0)
		roadLanes.erase(roadLanes.begin()+0);

	for(unsigned int l= 0; l < roadLanes.size(); l++)
	{
		for(unsigned int fp = 0; fp< roadLanes.at(l).fromIds.size(); fp++)
		{
			for(unsigned int in_l= 0; in_l < roadLanes.size(); in_l++)
			{
				bool bFound = false;
				for(unsigned int in_p = 0; in_p<roadLanes.at(in_l).points.size(); in_p++)
				{
					if(roadLanes.at(l).fromIds.at(fp) == roadLanes.at(in_l).points.at(in_p).id)
					{
						roadLanes.at(l).fromIds.at(fp) = roadLanes.at(in_l).points.at(in_p).laneId;
						bFound = true;
						break;
					}
				}
				if(bFound == true)
					break;
			}
		}
	}

	roadLanes.push_back(lane_obj);
	cout << "Lanes No = " << roadLanes.size() << endl;

	//map has one road segment
	RoadSegment roadSegment1;
	roadSegment1.id = 1;
	roadSegment1.Lanes = roadLanes;
	map.roadSegments.push_back(roadSegment1);


	for(unsigned int rs = 0; rs < map.roadSegments.size(); rs++)
	{
		//Link Lanes
		for(unsigned int i =0; i < map.roadSegments.at(rs).Lanes.size(); i++)
		{
			for(unsigned int j = 0 ; j < map.roadSegments.at(rs).Lanes.at(i).fromIds.size(); j++)
			{
				for(unsigned int l= 0; l < map.roadSegments.at(rs).Lanes.size(); l++)
				{
					if(map.roadSegments.at(rs).Lanes.at(l).id == map.roadSegments.at(rs).Lanes.at(i).fromIds.at(j))
					{
						map.roadSegments.at(rs).Lanes.at(i).fromLanes.push_back(&map.roadSegments.at(rs).Lanes.at(l));
					}
				}
			}

			for(unsigned int j = 0 ; j < map.roadSegments.at(rs).Lanes.at(i).toIds.size(); j++)
			{
				for(unsigned int l= 0; l < map.roadSegments.at(rs).Lanes.size(); l++)
				{
					if(map.roadSegments.at(rs).Lanes.at(l).id == map.roadSegments.at(rs).Lanes.at(i).toIds.at(j))
					{
						map.roadSegments.at(rs).Lanes.at(i).toLanes.push_back(&map.roadSegments.at(rs).Lanes.at(l));
					}
				}
			}

			for(unsigned int j = 0 ; j < map.roadSegments.at(rs).Lanes.at(i).points.size(); j++)
			{
				map.roadSegments.at(rs).Lanes.at(i).points.at(j).laneId = map.roadSegments.at(rs).Lanes.at(i).id;
				map.roadSegments.at(rs).Lanes.at(i).points.at(j).pLane  = &map.roadSegments.at(rs).Lanes.at(i);
			}
		}
	}

	//Link waypoints
	for(unsigned int rs = 0; rs < map.roadSegments.size(); rs++)
	{
		for(unsigned int i =0; i < map.roadSegments.at(rs).Lanes.size(); i++)
		{
			for(unsigned int p= 0; p < map.roadSegments.at(rs).Lanes.at(i).points.size(); p++)
			{

				//Don't do this, will screw up the tree trace back
//				for(unsigned int j = 0 ; j < map.roadSegments.at(rs).Lanes.at(i).points.at(p).fromIds.size(); j++)
//				{
//					map.roadSegments.at(rs).Lanes.at(i).points.at(p).pBacks.push_back(FindWaypoint(map.roadSegments.at(rs).Lanes.at(i).points.at(p).fromIds.at(j), map));
//				}

				for(unsigned int j = 0 ; j < map.roadSegments.at(rs).Lanes.at(i).points.at(p).toIds.size(); j++)
				{
					map.roadSegments.at(rs).Lanes.at(i).points.at(p).pFronts.push_back(FindWaypoint(map.roadSegments.at(rs).Lanes.at(i).points.at(p).toIds.at(j), map));
				}
			}
		}
	}
}

WayPoint* MappingHelpers::FindWaypoint(const int& id, RoadNetwork& map)
{
	for(unsigned int rs = 0; rs < map.roadSegments.size(); rs++)
	{
		for(unsigned int i =0; i < map.roadSegments.at(rs).Lanes.size(); i++)
		{
			for(unsigned int p= 0; p < map.roadSegments.at(rs).Lanes.at(i).points.size(); p++)
			{
				if(map.roadSegments.at(rs).Lanes.at(i).points.at(p).id == id)
					return &map.roadSegments.at(rs).Lanes.at(i).points.at(p);
			}
		}
	}

	return 0;
}

void MappingHelpers::ConstructRoadNetworkFromDataFiles(const std::string vectoMapPath, RoadNetwork& map)
{
	/**
	 * Exporting the center lines
	 */
	string laneLinesDetails = vectoMapPath + "point.csv";
	string center_lines_info = vectoMapPath + "dtlane.csv";
	string lane_info = vectoMapPath + "lane.csv";
	string node_info = vectoMapPath + "node.csv";

	AisanCenterLinesFileReader  center_lanes(center_lines_info);
	AisanLanesFileReader lanes(lane_info);
	AisanPointsFileReader points(laneLinesDetails);
	AisanNodesFileReader nodes(node_info);


	vector<AisanNodesFileReader::AisanNode> nodes_data;
	nodes.ReadAllData(nodes_data);

	vector<AisanLanesFileReader::AisanLane> lanes_data;
	lanes.ReadAllData(lanes_data);

	vector<AisanPointsFileReader::AisanPoints> points_data;
	points.ReadAllData(points_data);

	vector<AisanCenterLinesFileReader::AisanCenterLine> dt_data;
	center_lanes.ReadAllData(dt_data);


	ConstructRoadNetworkFromRosMessage(lanes_data, points_data, dt_data,GetTransformationOrigin(), map);


	WayPoint origin = GetFirstWaypoint(map);
//	WayPoint origin2 = GetFirstWaypoint(map);
//	WayPoint lastPoint2 = lastPoint;
//
//	llaToxyz(origin.pos, GPSPoint());
//	llaToxyz(lastPoint.pos, GPSPoint());
//
//	double distance = distance2points(origin.pos, lastPoint.pos);
//
	cout << origin.pos.ToString() ;
//
//
//	geo_pos_conv geo;
//	geo.set_plane(6);
//	geo.llh_to_xyz(origin2.pos.lon, origin2.pos.lat, 0);
//
//	origin2.pos.x = geo.x();
//	origin2.pos.y = geo.y();
//
//	geo.llh_to_xyz(lastPoint2.pos.lon, lastPoint2.pos.lat, 0);
//
//	lastPoint2.pos.x = geo.x();
//	lastPoint2.pos.y = geo.y();
//
//	double distance2 = distance2points(origin2.pos, lastPoint2.pos);
//	cout << origin2.pos.ToString() ;

}

void MappingHelpers::SaveTrajectoryLonLatToKMLFile(const string& fileName, const vector<vector<WayPoint> >& trajectory)
  {
  	vector<vector<string> > m_str_kml;

  	for(unsigned l = 0 ; l < trajectory.size(); l++)
  	{
  		vector<string>  lane_str_kml;
		for(unsigned k = 0 ; k < trajectory.at(l).size(); k++)
		{
			ostringstream gps_str;
			gps_str.precision(12);
			GPSPoint gps_p = trajectory.at(l).at(k).pos;
			gps_str << gps_p.x << "," << gps_p.y << "," << gps_p.z;
			lane_str_kml.push_back(gps_str.str());
		}
		m_str_kml.push_back(lane_str_kml);
  	}

  	if(m_str_kml.size() > 0)
  		DataRW::WriteKMLFile(fileName, m_str_kml);
  }

void MappingHelpers::GetWayPoint(const int& pid, const vector<AisanPointsFileReader::AisanPoints>& points, std::vector<WayPoint>& path)
{

	for(unsigned int p =0; p < points.size(); p++)
	{
		if(pid == points.at(p).PID)
		{
			WayPoint wp;
			wp.laneId = points.at(p).PID;
			wp.pos = GPSPoint(points.at(p).L, points.at(p).B, points.at(p).H, 0);
			path.push_back(wp);
			break;
		}
	}
}

bool MappingHelpers::GetWayPoint(const int& id, const int& laneID,const double& refVel, const int& did,
		const std::vector<UtilityHNS::AisanCenterLinesFileReader::AisanCenterLine>& dtpoints,
		const std::vector<UtilityHNS::AisanPointsFileReader::AisanPoints>& points,
		const GPSPoint& origin, WayPoint& way_point)
{

	for(unsigned int dtp =0; dtp < dtpoints.size(); dtp++)
	{
		if(dtpoints.at(dtp).DID == did)
		{
			for(unsigned int p =0; p < points.size(); p++)
			{
				if(dtpoints.at(dtp).PID == points.at(p).PID)
				{
					WayPoint wp;
					wp.id = id;
					wp.laneId = laneID;
					wp.v = refVel;
					double integ_part = points.at(p).L;
					double deg = trunc(points.at(p).L);
					double min = trunc((points.at(p).L - deg) * 100.0) / 60.0;
					double sec = modf((points.at(p).L - deg) * 100.0, &integ_part)/36.0;
					double L =  deg + min + sec;

					deg = trunc(points.at(p).B);
					min = trunc((points.at(p).B - deg) * 100.0) / 60.0;
					sec = modf((points.at(p).B - deg) * 100.0, &integ_part)/36.0;
					double B = deg + min + sec;

					wp.pos = GPSPoint(points.at(p).Ly + origin.x, points.at(p).Bx + origin.y, points.at(p).H + origin.z, dtpoints.at(dtp).Dir);

					wp.pos.lat = points.at(p).L;
					wp.pos.lon = points.at(p).B;
					wp.pos.alt = points.at(p).H;
					wp.pos.dir = dtpoints.at(dtp).Dir;

					way_point = wp;
					return 1;
				}
			}
		}
	}

	return 0;
}

void MappingHelpers::WriteKML(const string& kmlFile, const string& kmlTemplat, RoadNetwork& map)
{
	//First, Get the main element
	TiXmlElement* pHeadElem = 0;
	TiXmlElement* pElem = 0;

	TiXmlDocument doc(kmlTemplat);
	if(!doc.LoadFile())
	{
		cout << doc.ErrorDesc() << endl;
	}
	else
		pElem = doc.FirstChildElement();

	pHeadElem = GetHeadElement(pElem);

	vector<Lane> allLaneLinks;
	vector<RoadSegment> roadLinks;

	for(unsigned int j=0; j< map.roadSegments.size(); j++)
	{
		allLaneLinks.insert(allLaneLinks.end(), map.roadSegments.at(j).Lanes.begin(),
				map.roadSegments.at(j).Lanes.end());
		roadLinks.push_back(map.roadSegments.at(j));
	}

	SetLaneLinksList(pHeadElem, allLaneLinks);
	//SetRoadLinksList(pHeadElem, roadLinks);

	doc.SaveFile(kmlFile.c_str());
}

TiXmlElement* MappingHelpers::GetHeadElement(TiXmlElement* pMainElem)
{
	TiXmlElement* pElem = pMainElem;
	if(pElem)
		pElem = pElem->FirstChildElement("Folder");
	if(pElem && pElem->FirstChildElement("Folder"))
		pElem = pElem->FirstChildElement("Folder");
	if(pElem && pElem->FirstChildElement("Document"))
		pElem = pElem->FirstChildElement("Document");

	if(!pElem)
		return 0;
	return pElem;
}

TiXmlElement* MappingHelpers::GetDataFolder(const string& folderName, TiXmlElement* pMainElem)
{
	if(!pMainElem) return 0;

	TiXmlElement* pElem = pMainElem->FirstChildElement("Folder");

	string folderID="";
	for(; pElem; pElem=pElem->NextSiblingElement())
	{
		folderID="";
		if(pElem->FirstChildElement("name")->GetText()) //Map Name
			folderID = pElem->FirstChildElement("name")->GetText();
		if(folderID.compare(folderName)==0)
			return pElem;
	}
	return 0;
}

void MappingHelpers::SetLaneLinksList(TiXmlElement* pElem, vector<Lane>& lanes)
{
	TiXmlElement* pLaneLinks = GetDataFolder("LaneLinks", pElem);
	TiXmlNode* pE = pLaneLinks->FirstChild("Folder");
	TiXmlNode* pN = pE;
	TiXmlText * pText = 0;
	TiXmlElement* pElement = 0;
	int roadId = -1;
	int roadsCount = 0;

	if(lanes.size() ==0)
		pE->Clear();

	for(unsigned int i=0; i< lanes.size(); i++)
	{
		Lane* pLane = &lanes.at(i);

		if(pLane->roadId != roadId)
		{
			roadId = pLane->roadId;
			roadsCount++;
		}

		if(i>0)
			pN = pE->Clone();

		ostringstream name, desc;
		name << "LL_" << pLane->id;
		pElement = pN->FirstChild("name")->ToElement();
		pText = new TiXmlText(name.str());
		pElement->Clear();
		pElement->LinkEndChild(pText);

		desc << "LL_" << pLane->id << "_RL_" << pLane->roadId << "_NUM_" << pLane->num << "_From_";
		for(unsigned int j=0; j< pLane->fromIds.size(); j++)
			desc << pLane->fromIds.at(j) << "_";

		desc << "To";
		for(unsigned int j=0; j< pLane->toIds.size(); j++)
			desc << "_" << pLane->toIds.at(j);

		pElement = pN->FirstChild("description")->ToElement();
		pText = new TiXmlText(desc.str());
		pElement->Clear();
		pElement->LinkEndChild(pText);

		ostringstream style;
		pElement = pN->FirstChild("Placemark")->FirstChild("styleUrl")->ToElement();
		style << "#mms_route_line_" << roadsCount%4;
		pText = new TiXmlText(style.str());
		pElement->Clear();
		pElement->LinkEndChild(pText);


		pElement = pN->FirstChild("Placemark")->FirstChild("LineString")->FirstChild("coordinates")->ToElement();

		ostringstream val;
		val.precision(18);

		for(unsigned int j =0; j < pLane->points.size() ; j++)
		{
			GPSPoint p = pLane->points.at(j).pos;
			val << p.lat << "," << p.lon << "," << p.alt <<  " ";
		}

		pText = new TiXmlText(val.str());
		pElement->Clear();
		pElement->LinkEndChild(pText);
		if(i>0)
			pLaneLinks->InsertEndChild(*pN);
	}
}

Lane* MappingHelpers::GetClosestLaneFromMap(const WayPoint& pos, RoadNetwork& map, const double& distance)
{
	vector<pair<double, Lane*> > laneLinksList;
	double d = 0;
	double min_d = 9999999999;
	for(unsigned int j=0; j< map.roadSegments.size(); j ++)
	{
		for(unsigned int k=0; k< map.roadSegments.at(j).Lanes.size(); k ++)
		{
			//Lane* pLane = &pEdge->lanes.at(k);
			 d = 0;
			min_d = 9999999999;
			for(unsigned int pindex=0; pindex< map.roadSegments.at(j).Lanes.at(k).points.size(); pindex ++)
			{

				d = distance2points(map.roadSegments.at(j).Lanes.at(k).points.at(pindex).pos, pos.pos);
				if(d < min_d)
					min_d = d;
			}

			if(min_d < distance)
				laneLinksList.push_back(make_pair(min_d, &map.roadSegments.at(j).Lanes.at(k)));
		}
	}

	if(laneLinksList.size() == 0) return 0;

	d = 0, min_d = 999999999;
	Lane* closest_lane = 0;
	for(unsigned int i = 0; i < laneLinksList.size(); i++)
	{
		d = PlanningHelpers::GetPerpDistanceToTrajectorySimple(laneLinksList.at(i).second->points, pos);
		if(d == 0 && laneLinksList.at(i).first != 0)
			continue;

		if(abs(d)<min_d)
		{
			min_d = abs(d);
			closest_lane = laneLinksList.at(i).second;
		}
	}

	return closest_lane;
}

WayPoint MappingHelpers::GetFirstWaypoint(RoadNetwork& map)
{
	for(unsigned int j=0; j< map.roadSegments.size(); j ++)
	{
		for(unsigned int k=0; k< map.roadSegments.at(j).Lanes.size(); k ++)
		{
			for(unsigned int pindex=0; pindex< map.roadSegments.at(j).Lanes.at(k).points.size(); pindex ++)
			{
				WayPoint fp =  map.roadSegments.at(j).Lanes.at(k).points.at(pindex);
				return fp;
			}
		}
	}

	return WayPoint();
}

void MappingHelpers::llaToxyz(GPSPoint& lla_p, const GPSPoint& origin)
{
	projPJ pj_latlong, pj_utm;
	pj_latlong = pj_init_plus("+proj=latlong +ellps=WGS84");
	pj_utm = pj_init_plus("+proj=utm +zone=53 +ellps=WGS84 +datum=WGS84");

	GPSPoint p = lla_p;

	p.x = lla_p.lon * DEG2RAD;
	p.y = lla_p.lat * DEG2RAD;

	if(pj_latlong != 0 && pj_utm !=0 )
	{
		pj_transform(pj_latlong, pj_utm, 1, 1, &p.y, &p.x, &p.z);
		p.x -= origin.x;
		p.y -= origin.y;
		p.z -= origin.z;
	}

	lla_p = p;

}

void MappingHelpers::xyzTolla(GPSPoint& xyz_p, const GPSPoint& origin)
{
	projPJ pj_latlong, pj_utm;
	pj_latlong = pj_init_plus("+proj=latlong +ellps=WGS84");
	pj_utm = pj_init_plus("+proj=utm +ellps=WGS84 +datum=WGS84 +units=m");

	GPSPoint p = xyz_p;

	if(pj_latlong != 0 && pj_utm !=0 )
	{
		pj_transform(pj_utm,pj_latlong, 1, 1, &p.lon, &p.lat, &p.alt);
		p.lon = p.lon * RAD2DEG;
		p.lat = p.lat * RAD2DEG;

		p.lon -= origin.lon;
		p.lat -= origin.lat;
		p.alt -= origin.alt;
	}

	xyz_p = p;

}

void MappingHelpers::GetUniqueNextLanes(const Lane* l,  const vector<Lane*>& traversed_lanes, vector<Lane*>& lanes_list)
{
	if(!l) return;

	for(unsigned int i=0; i< l->toLanes.size(); i++)
	{
		bool bFound = false;
		for(unsigned int j = 0; j < traversed_lanes.size(); j++)
		if(l->toLanes.at(i)->id == traversed_lanes.at(j)->id)
		{
			bFound = true;
			break;
		}

		if(!bFound)
			lanes_list.push_back(l->toLanes.at(i));
	}
}

Lane* MappingHelpers::GetLaneFromPath(const WayPoint& currPos, const std::vector<WayPoint>& currPath)
{
	if(currPath.size() < 1) return 0;

	int closest_index = PlanningHelpers::GetClosestNextPointIndex(currPath, currPos);

	return currPath.at(closest_index).pLane;
}

} /* namespace PlannerHNS */
