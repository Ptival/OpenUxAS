// ===============================================================================
// Authors: AFRL/RQQA
// Organization: Air Force Research Laboratory, Aerospace Systems Directorate, Power and Control Division
// 
// Copyright (c) 2017 Government of the United State of America, as represented by
// the Secretary of the Air Force.  No copyright is claimed in the United States under
// Title 17, U.S. Code.  All Other Rights Reserved.
// ===============================================================================

/* 
 * File:   DAIDALUS_WCV_Detection.cpp
 * Author: SeanR
 *
 *
 *
 * <Service Type="DAIDALUS_WCV_Detection" OptionString="Option_01" OptionInt="36" />
 * 
 */

// include header for this service
#include "DAIDALUS_WCV_Detection.h"

//include for KeyValuePair LMCP Message
#include "afrl/cmasi/KeyValuePair.h" //this is an exemplar
#include "afrl/cmasi/AirVehicleState.h"
#include "BandsRegion.h"
#include "Interval.h"
#include "larcfm/DAIDALUS/DAIDALUSConfiguration.h"
#include "larcfm/DAIDALUS/WellClearViolationIntervals.h"

#include <iostream>     // std::cout, cerr, etc
#include <cmath>    //cmath::cos, sin, etc
#include <string>   //std::to_string etc


// convenience definitions for the option strings
#define STRING_XML_OPTION_STRING "OptionString"
#define STRING_XML_OPTION_INT "OptionInt"

#define STRING_XML_LOOKAHEADTIME "LookAheadTime"
#define STRING_XML_LEFTTRACK "LeftTrack"
#define STRING_XML_RIGHTTRACK "RightTrack"
#define STRING_XML_MINGROUNDSPEED "MinGroundSpeed"
#define STRING_XML_MAXGROUNDSPEED "MaxGroundSpeed"
#define STRING_XML_MINVERTICALSPEED "MinVerticalSpeed"
#define STRING_XML_MAXVERTICALSPEED "MaxVerticalSpeed"
#define STRING_XML_MINALTITUDE "MinAltitude"
#define STRING_XML_MAXALTITUDE "MaxAltitude"
#define STRING_XML_TRACKSTEP "TrackStep"
#define STRING_XML_GROUNDSPEEDSTEP "GroundSpeedStep"
#define STRING_XML_VERTICALSPEEDSTEP "VerticalSpeedStep"
#define STRING_XML_ALTITUDESTEP "AltitudeStep"
#define STRING_XML_HORIZONTALACCELERATION "HorizontalAcceleration"
#define STRING_XML_VERTICALACCELERATION "VerticalAcceleration"
#define STRING_XML_TURNRATE "TurnRate"
#define STRING_XML_BANKANGLE "BankAngle"
#define STRING_XML_VERTICALRATE "VerticalRate"
#define STRING_XML_RECOVERYSTABILITYTIME "RecoveryStabilityTime"
#define STRING_XML_MINHORIZONTALRECOVERY "MinHorizontalRecovery"
#define STRING_XML_MINVERTICALRECOVERY "MinVerticalRecovery"
#define STRING_XML_ISRECOVERYTRACK "isRecoveryTrack"
#define STRING_XML_ISRECOVERYGROUNDSPEED "isRecoveryGroundSpeed"
#define STRING_XML_ISRECOVERYVERTICALSPEED "isRecoveryVerticalSpeed"
#define STRING_XML_ISRECOVERYALTITUDE "isRecoveryAltitude"
#define STRING_XML_ISCOLLISIONAVOIDANCE "isCollisionAvoidance"
#define STRING_XML_COLLISIONAVOIDANCEFACTOR "CollisionAvoidanceFactor"
#define STRING_XML_HORIZONTALNMAC "HorizontalNMAC"
#define STRING_XML_VERTICALNMAC "VerticalNMAC"
#define STRING_XML_HORIZONTALCONTOURTHRESHOLD "HorizontalContourThreshold"




// useful definitions
#define MILLISECONDTOSECOND 1.0/1000.0

//todo add units to variable names
namespace {
    void makeVelocityXYZ(double u, double v, double w, double Phi_rad, double Theta_rad, double Psi_rad, double& velocityX, double& velocityY, 
            double& velocityZ)
    {
        velocityX = std::cos(Theta_rad)*std::cos(Psi_rad)*u + (std::sin(Phi_rad)*std::sin(Theta_rad)*std::cos(Psi_rad)- 
                std::cos(Phi_rad)*std::sin(Psi_rad))*v + (std::cos(Phi_rad)*std::sin(Theta_rad)*std::cos(Psi_rad) + 
                std::sin(Phi_rad)*std::sin(Psi_rad))*w;
        velocityY = std::cos(Theta_rad)*std::sin(Psi_rad)*u + (std::sin(Phi_rad)*std::sin(Theta_rad)*std::sin(Psi_rad)+ 
                std::cos(Phi_rad)*std::cos(Psi_rad))*v + (std::cos(Phi_rad)*std::sin(Theta_rad)*std::sin(Psi_rad)- 
                std::sin(Phi_rad)*std::cos(Psi_rad))*w;
        velocityZ = -std::sin(Theta_rad)*u + std::sin(Phi_rad)*std::cos(Theta_rad)*v + std::cos(Phi_rad)*std::cos(Theta_rad)*w;
    }
}

// namespace definitions
namespace uxas  // uxas::
{
namespace service   // uxas::service::
{   

// this entry registers the service in the service creation registry
DAIDALUS_WCV_Detection::ServiceBase::CreationRegistrar<DAIDALUS_WCV_Detection>
DAIDALUS_WCV_Detection::s_registrar(DAIDALUS_WCV_Detection::s_registryServiceTypeNames());

//create a DAIDALUS object

// service constructor
DAIDALUS_WCV_Detection::DAIDALUS_WCV_Detection()
: ServiceBase(DAIDALUS_WCV_Detection::s_typeName(), DAIDALUS_WCV_Detection::s_directoryName()) { }; 

// service destructor
DAIDALUS_WCV_Detection::~DAIDALUS_WCV_Detection() { };

bool DAIDALUS_WCV_Detection::configure(const pugi::xml_node& ndComponent)
{
    bool isSuccess(true);

    // process options from the XML configuration node:
    if (!ndComponent.attribute(STRING_XML_LOOKAHEADTIME).empty())
    {
       m_lookahead_time_s = ndComponent.attribute(STRING_XML_LOOKAHEADTIME).as_double();
       if (m_lookahead_time_s > 0.0)
       {
           m_daa.parameters.setLookaheadTime(m_lookahead_time_s, "s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_LEFTTRACK).empty())
    {
       m_left_trk_deg = ndComponent.attribute(STRING_XML_LEFTTRACK).as_double();
       if (m_left_trk_deg > 0.0 && m_left_trk_deg <= 180.0)
       {
           m_daa.parameters.setLeftTrack(m_left_trk_deg, "deg");
       }
    }
    if (!ndComponent.attribute(STRING_XML_RIGHTTRACK).empty())
    {
       m_right_trk_deg = ndComponent.attribute(STRING_XML_RIGHTTRACK).as_double();
       if (m_right_trk_deg >0.0 && m_right_trk_deg <=180.0)
       {
           m_daa.parameters.setRightTrack(m_right_trk_deg, "deg");
       }
    }
    if (!ndComponent.attribute(STRING_XML_MAXGROUNDSPEED).empty())
    {
       m_max_gs_mps = ndComponent.attribute(STRING_XML_MAXGROUNDSPEED).as_double();
       if (m_max_gs_mps > 0.0)
       {
           m_daa.parameters.setMaxGroundSpeed(m_max_gs_mps, "m/s");
       }
    }
        if (!ndComponent.attribute(STRING_XML_MINGROUNDSPEED).empty())
    {
       m_min_gs_mps = ndComponent.attribute(STRING_XML_MINGROUNDSPEED).as_double();
       if (m_min_gs_mps >= 0.0 && m_min_gs_mps < m_max_gs_mps)
       {
           m_daa.parameters.setMinGroundSpeed(m_min_gs_mps, "m/s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_MAXVERTICALSPEED).empty())
    {
       m_max_vs_mps = ndComponent.attribute(STRING_XML_MAXVERTICALSPEED).as_double();
       m_daa.parameters.setMaxVerticalSpeed(m_max_vs_mps, "m/s");
    }
        if (!ndComponent.attribute(STRING_XML_MINVERTICALSPEED).empty())
    {
       m_min_vs_mps = ndComponent.attribute(STRING_XML_MINVERTICALSPEED).as_double();
       if (m_min_vs_mps < m_max_vs_mps)
       {
           m_daa.parameters.setMinVerticalSpeed(m_min_vs_mps, "m/s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_MAXALTITUDE).empty())
    {
       m_max_alt_m = ndComponent.attribute(STRING_XML_MAXALTITUDE).as_double();
       m_daa.parameters.setMaxAltitude(m_max_alt_m, "m");
    }
        if (!ndComponent.attribute(STRING_XML_MINALTITUDE).empty())
    {
       m_min_alt_m = ndComponent.attribute(STRING_XML_MINALTITUDE).as_double();
       if (m_min_alt_m < m_max_alt_m)
       {
           m_daa.parameters.setMinAltitude(m_min_alt_m, "m");
       }
    }
    if (!ndComponent.attribute(STRING_XML_TRACKSTEP).empty())
    {
       m_trk_step_deg = ndComponent.attribute(STRING_XML_TRACKSTEP).as_double();
       if (m_trk_step_deg > 0.0)
       {
           m_daa.parameters.setTrackStep(m_trk_step_deg, "deg");
       }
    }
    if (!ndComponent.attribute(STRING_XML_GROUNDSPEEDSTEP).empty())
    {
       m_gs_step_mps = ndComponent.attribute(STRING_XML_GROUNDSPEEDSTEP).as_double();
       if (m_gs_step_mps > 0.0)
       {
           m_daa.parameters.setGroundSpeedStep(m_gs_step_mps, "m/s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_VERTICALSPEEDSTEP).empty())
    {
       m_vs_step_mps = ndComponent.attribute(STRING_XML_VERTICALSPEEDSTEP).as_double();
       if (m_vs_step_mps > 0.0)
       {
           m_daa.parameters.setVerticalSpeedStep(m_vs_step_mps, "m/s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_ALTITUDESTEP).empty())
    {
       m_alt_step_m = ndComponent.attribute(STRING_XML_ALTITUDESTEP).as_double();
       if (m_alt_step_m > 0.0)
       {
           m_daa.parameters.setAltitudeStep(m_alt_step_m, "m");
       }
    }
    if (!ndComponent.attribute(STRING_XML_HORIZONTALACCELERATION).empty())
    {
       m_horizontal_accel_mpsps = ndComponent.attribute(STRING_XML_HORIZONTALACCELERATION).as_double();
       if (m_horizontal_accel_mpsps >= 0.0)
       {
           m_daa.parameters.setHorizontalAcceleration(m_horizontal_accel_mpsps, "m/s^2");
       }
    }
    if (!ndComponent.attribute(STRING_XML_VERTICALACCELERATION).empty())
    {
       m_vertical_accel_G = ndComponent.attribute(STRING_XML_VERTICALACCELERATION).as_double();
       if (m_vertical_accel_G >= 0.0)
       {
           m_daa.parameters.setVerticalAcceleration(m_vertical_accel_G, "G");
       }
    }
    if (!ndComponent.attribute(STRING_XML_TURNRATE).empty())
    {
       m_turn_rate_degps = ndComponent.attribute(STRING_XML_TURNRATE).as_double();
       if (m_turn_rate_degps >= 0.0)
       {
           m_daa.parameters.setTurnRate(m_turn_rate_degps, "deg/s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_BANKANGLE).empty())
    {
       m_bank_angle_deg = ndComponent.attribute(STRING_XML_BANKANGLE).as_double();
       if (m_bank_angle_deg >= 0.0 && m_turn_rate_degps != 0.0)
       {
           m_daa.parameters.setBankAngle(m_bank_angle_deg, "deg");
       }
    }
    if (!ndComponent.attribute(STRING_XML_VERTICALRATE).empty())
    {
       m_vertical_rate_mps = ndComponent.attribute(STRING_XML_VERTICALRATE).as_double();
       if (m_vertical_rate_mps >= 0.0)
       {
           m_daa.parameters.setVerticalRate(m_vertical_rate_mps, "m/s");
       }
    }    
    if (!ndComponent.attribute(STRING_XML_RECOVERYSTABILITYTIME).empty())
    {
       m_recovery_stability_time_s = ndComponent.attribute(STRING_XML_RECOVERYSTABILITYTIME).as_double();
       if (m_recovery_stability_time_s >= 0.0)
       {
           m_daa.parameters.setRecoveryStabilityTime(m_recovery_stability_time_s, "s");
       }
    }
    if (!ndComponent.attribute(STRING_XML_ISRECOVERYTRACK).empty())
    {
       m_recovery_trk_bool = ndComponent.attribute(STRING_XML_ISRECOVERYTRACK).as_bool();
       m_daa.parameters.setRecoveryTrackBands(m_recovery_trk_bool);
    }
    if (!ndComponent.attribute(STRING_XML_ISRECOVERYGROUNDSPEED).empty())
    {
       m_recovery_gs_bool = ndComponent.attribute(STRING_XML_ISRECOVERYGROUNDSPEED).as_bool();
       m_daa.parameters.setRecoveryGroundSpeedBands(m_recovery_gs_bool);
    }
    if (!ndComponent.attribute(STRING_XML_ISRECOVERYVERTICALSPEED).empty())
    {
       m_recovery_vs_bool = ndComponent.attribute(STRING_XML_ISRECOVERYVERTICALSPEED).as_bool();
       m_daa.parameters.setRecoveryVerticalSpeedBands(m_recovery_vs_bool);
    }
    if (!ndComponent.attribute(STRING_XML_ISRECOVERYALTITUDE).empty())
    {
       m_recovery_alt_bool = ndComponent.attribute(STRING_XML_ISRECOVERYALTITUDE).as_bool();
       m_daa.parameters.setRecoveryAltitudeBands(m_recovery_alt_bool);
    }
    if (!ndComponent.attribute(STRING_XML_ISCOLLISIONAVOIDANCE).empty())
    {
       m_ca_bands_bool = ndComponent.attribute(STRING_XML_ISCOLLISIONAVOIDANCE).as_bool();
       m_daa.parameters.setCollisionAvoidanceBands(m_ca_bands_bool);
    }
    if (!ndComponent.attribute(STRING_XML_COLLISIONAVOIDANCEFACTOR).empty())
    {
       m_ca_factor = ndComponent.attribute(STRING_XML_COLLISIONAVOIDANCEFACTOR).as_double();
       if (m_ca_factor > 0.0 && m_ca_factor <= 1.0)
       {
           m_daa.parameters.setCollisionAvoidanceBandsFactor(m_ca_factor);
       }
    }
    if (!ndComponent.attribute(STRING_XML_HORIZONTALNMAC).empty())
    {
        m_horizontal_nmac_m = ndComponent.attribute(STRING_XML_HORIZONTALNMAC).as_double();
       m_daa.parameters.setHorizontalNMAC(m_horizontal_nmac_m, "m");
    }
        if (!ndComponent.attribute(STRING_XML_MINHORIZONTALRECOVERY).empty())
    {
       m_min_horizontal_recovery_m = ndComponent.attribute(STRING_XML_MINHORIZONTALRECOVERY).as_double();
       if (m_min_horizontal_recovery_m > 0.0 && m_min_horizontal_recovery_m >= m_horizontal_nmac_m)
       {
           m_daa.parameters.setMinHorizontalRecovery(m_min_horizontal_recovery_m, "m");
       }
    }
    if (!ndComponent.attribute(STRING_XML_VERTICALNMAC).empty())
    {
       m_vertical_nmac_m = ndComponent.attribute(STRING_XML_VERTICALNMAC).as_double();
       m_daa.parameters.setVerticalNMAC(m_vertical_nmac_m, "m");
    }
        if (!ndComponent.attribute(STRING_XML_MINVERTICALRECOVERY).empty())
    {
       m_min_vertical_recovery_m = ndComponent.attribute(STRING_XML_MINVERTICALRECOVERY).as_double();
       if (m_min_vertical_recovery_m > 0.0 && m_min_vertical_recovery_m >= m_vertical_nmac_m)
       {
           m_daa.parameters.setMinVerticalRecovery(m_min_vertical_recovery_m, "m");
       }
    }

    if (!ndComponent.attribute(STRING_XML_HORIZONTALCONTOURTHRESHOLD).empty())
    {
       m_contour_thr_deg = ndComponent.attribute(STRING_XML_HORIZONTALCONTOURTHRESHOLD).as_double();
       if (m_contour_thr_deg >= 0.0 && m_contour_thr_deg <= 180.0)
       m_daa.parameters.setHorizontalContourThreshold(m_contour_thr_deg, "deg");
    }
  
    addSubscriptionAddress(afrl::cmasi::AirVehicleState::Subscription);
    std::cout << "Successfully subscribed to AirVehicleState from DAIDALUS_WCV_Detection." << std::endl;
    
    return (isSuccess);
}

bool DAIDALUS_WCV_Detection::initialize()
{
    // perform any required initialization before the service is started

    
    //std::cout << "*** INITIALIZING:: Service[" << s_typeName() << "] Service Id[" << m_serviceId << "] with working directory [" << m_workDirectoryName << "] *** " << std::endl;
    
    return (true);
}

bool DAIDALUS_WCV_Detection::start() 
{
    // perform any actions required at the time the service starts
    //std::cout << "*** STARTING:: Service[" << s_typeName() << "] Service Id[" << m_serviceId << "] with working directory [" << m_workDirectoryName << "] *** " << std::endl;
    std::shared_ptr<larcfm::DAIDALUS::DAIDALUSConfiguration> DetectionConfiguration = std::make_shared<larcfm::DAIDALUS::DAIDALUSConfiguration>();
    DetectionConfiguration->setLookAheadTime(m_daa.parameters.getLookaheadTime("s"));
    DetectionConfiguration->setLeftTrack(m_daa.parameters.getLeftTrack("deg"));
    DetectionConfiguration->setRightTrack(m_daa.parameters.getRightTrack("deg"));
    DetectionConfiguration->setMaxGroundSpeed(m_daa.parameters.getMaxGroundSpeed("mps"));
    DetectionConfiguration->setMinGroundSpeed(m_daa.parameters.getMinGroundSpeed("mps"));
    DetectionConfiguration->setMaxVerticalSpeed(m_daa.parameters.getMaxVerticalSpeed("mps"));
    DetectionConfiguration->setMinVerticalSpeed(m_daa.parameters.getMinVerticalSpeed("mps"));
    DetectionConfiguration->setMaxAltitude(m_daa.parameters.getMaxAltitude("m"));
    DetectionConfiguration->setMinAltitude(m_daa.parameters.getMinAltitude("m"));
    DetectionConfiguration->setTrackStep(m_daa.parameters.getTrackStep("deg"));
    DetectionConfiguration->setGroundSpeedStep(m_daa.parameters.getGroundSpeedStep("mps"));
    DetectionConfiguration->setVerticalSpeedStep(m_daa.parameters.getVerticalSpeedStep("mps"));
    DetectionConfiguration->setAltitudeStep(m_daa.parameters.getAltitudeStep("m"));
    DetectionConfiguration->setHorizontalAcceleration(m_daa.parameters.getHorizontalAcceleration("m/s^2"));
    DetectionConfiguration->setVerticalAcceleration(m_daa.parameters.getVerticalAcceleration("G"));
    DetectionConfiguration->setTurnRate(m_daa.parameters.getTurnRate("deg/s"));
    DetectionConfiguration->setBankAngle(m_daa.parameters.getBankAngle("deg"));
    DetectionConfiguration->setVerticalRate(m_daa.parameters.getVerticalRate("mps"));
    DetectionConfiguration->setRecoveryStabilityTime(m_daa.parameters.getRecoveryStabilityTime("s"));
    DetectionConfiguration->setIsRecoveryTrackBands(m_daa.parameters.isEnabledRecoveryTrackBands());
    DetectionConfiguration->setIsRecoveryGroundSpeedBands(m_daa.parameters.isEnabledRecoveryGroundSpeedBands());
    DetectionConfiguration->setIsRecoveryVerticalSpeedBands(m_daa.parameters.isEnabledRecoveryVerticalSpeedBands());
    DetectionConfiguration->setIsRecoveryAltitudeBands(m_daa.parameters.isEnabledRecoveryAltitudeBands());
    DetectionConfiguration->setIsCollisionAvoidanceBands(m_daa.parameters.isEnabledCollisionAvoidanceBands());
    DetectionConfiguration->setHorizontalNMAC(m_daa.parameters.getHorizontalNMAC("m"));
    DetectionConfiguration->setMinHorizontalRecovery(m_daa.parameters.getMinHorizontalRecovery("m"));
    DetectionConfiguration->setVerticalNMAC(m_daa.parameters.getVerticalNMAC("m"));
    DetectionConfiguration->setMinVerticalRecovery(m_daa.parameters.getMinVerticalRecovery("m"));
    DetectionConfiguration->setHorizontalContourThreshold(m_daa.parameters.getHorizontalContourThreshold("m"));
    sendSharedLmcpObjectBroadcastMessage(DetectionConfiguration);   
    return (true);
};

bool DAIDALUS_WCV_Detection::terminate()
{
    // perform any action required during service termination, before destructor is called.
    std::cout << "*** TERMINATING:: Service[" << s_typeName() << "] Service Id[" << m_serviceId << "] with working directory [" << 
            m_workDirectoryName << "] *** " << std::endl;    
    return (true);
}

bool DAIDALUS_WCV_Detection::processReceivedLmcpMessage(std::unique_ptr<uxas::communications::data::LmcpMessage> receivedLmcpMessage)
{
    if (afrl::cmasi::isAirVehicleState(receivedLmcpMessage->m_object))
    {
        m_nogo_trk_deg.clear();
        m_nogo_gs_mps.clear();
        m_nogo_vs_mps.clear();
        m_nogo_alt_m.clear();
        //receive message
        std::shared_ptr<afrl::cmasi::AirVehicleState> airVehicleState = std::static_pointer_cast<afrl::cmasi::AirVehicleState> (receivedLmcpMessage->m_object);
        std::cout << "DAIDALUS_WCV_Detection has received an AirVehicleState at " << airVehicleState->getTime() <<" ms--from Entity " << 
                airVehicleState->getID() << std::endl;
        //handle message
        std::unordered_map<int64_t, double> detectedViolations;        
        //add air vehicle message state to the Daidalus Object
        MydaidalusPackage vehicleInfo;  
        vehicleInfo.m_daidalusPosition = larcfm::Position::makeLatLonAlt(airVehicleState->getLocation()->getLatitude(), "deg",
                                         airVehicleState->getLocation()->getLongitude(), "deg", airVehicleState->getLocation()->getAltitude(), "m");      
        float u_mps = airVehicleState->getU();
        float v_mps = airVehicleState->getV();
        float w_mps = airVehicleState->getW();
        float Phi_deg = airVehicleState->getRoll();
        float Theta_deg = airVehicleState->getPitch();
        float Psi_deg = airVehicleState->getHeading();
        double velocityX_mps, velocityY_mps, velocityZ_mps;
        makeVelocityXYZ(u_mps, v_mps, w_mps, n_Const::c_Convert::toRadians(Phi_deg), n_Const::c_Convert::toRadians(Theta_deg), 
                n_Const::c_Convert::toRadians(Psi_deg), velocityX_mps, velocityY_mps, velocityZ_mps);
        // DAIDALUS expects ENUp reference while UxAS internally used NEDown--covert UxAS velocities to DAIDALUS velocities
        double daidalusVelocityZ_mps = -velocityZ_mps;    //add a comment for why
        double daidalusVelocityX_mps = velocityY_mps;
        double daidalusVelocityY_mps = velocityX_mps;
        vehicleInfo.m_daidalusVelocity = larcfm::Velocity::makeVxyz(daidalusVelocityX_mps, daidalusVelocityY_mps, "m/s", daidalusVelocityZ_mps, "m/s");
        vehicleInfo.m_daidalusTime_s = airVehicleState->getTime()*MILLISECONDTOSECOND; // conversion from UxAS representation of time in milliseconds to DAIDALUS representation fo time in seconds
        // DAIDALUS_WCV_Detection::m_entityId is the ID of the ownship   --TODO delete this comment before commiting to git repository     
        m_daidalusVehicleInfo[airVehicleState->getID()] = vehicleInfo;
        // Conditional check for appropriateness off a well clear violation check-- 2 known vehicle states including the ownship
        if (m_daidalusVehicleInfo.size()>1 && m_daidalusVehicleInfo.count(m_entityId)>0)    
        { 
            m_daa.setOwnshipState(std::to_string(m_entityId), m_daidalusVehicleInfo[m_entityId].m_daidalusPosition, 
                m_daidalusVehicleInfo[m_entityId].m_daidalusVelocity, m_daidalusVehicleInfo[m_entityId].m_daidalusTime_s); //set DAIDALUS object ownship state
            for (const auto& vehiclePackagedInfo : m_daidalusVehicleInfo)
            {
                //add intruder traffic state to DAIDALUS object
                if (vehiclePackagedInfo.first!=m_entityId) //--TODO add staleness check to this statement or put check on outer most if
                    {
                        m_daa.addTrafficState(std::to_string(vehiclePackagedInfo.first), vehiclePackagedInfo.second.m_daidalusPosition, 
                                vehiclePackagedInfo.second.m_daidalusVelocity, vehiclePackagedInfo.second.m_daidalusTime_s);
                        //std::cout << "Added Entity " << it_intruderId->first << " as an intruder to Entity " << m_entityId << std::endl; --TODO delete before commiting to repository
                    }
            }
            //std::cout << "Number of aircraft according to DAIDALUS: " <<m_daa.numberOfAircraft() << std::endl;--  TODO delete
            if (m_daa.numberOfAircraft()>1) //Perform well_clear violation check if DAIDALUS object contains ownship and at least one intruder traffic state
            {
                larcfm::KinematicMultiBands m_daa_bands;
                m_daa.kinematicMultiBands(m_daa_bands);
                for (int intruderIndex = 1; intruderIndex<=m_daa.numberOfAircraft()-1; ++intruderIndex)
                {
                    double timeToViolation_s = m_daa.timeToViolation(intruderIndex);
                    if (timeToViolation_s != PINFINITY && timeToViolation_s != NaN)
                    { 
                        detectedViolations[std::stoi(m_daa.getAircraftState(intruderIndex).getId(),nullptr,10)] = timeToViolation_s;
                        //std::cout << "Collision with intruder " <<m_daa.getAircraftState(intruderIndex).getId() << " in " << timeToViolation << " seconds" << std::endl;--TODO delete
                    }
                }
                //send out response
                //std::cout << "Number of aircraft according to DAIDALUS: " << m_daa.numberOfAircraft() << std::endl;--TODO delete
                if (!detectedViolations.empty())
                {
                    std::shared_ptr<larcfm::DAIDALUS::WellClearViolationIntervals>  nogo_ptr = 
                            std::make_shared<larcfm::DAIDALUS::WellClearViolationIntervals>();
                    for (int ii = 0; ii < m_daa_bands.trackLength(); ii++)
                    {
                        std::unique_ptr<larcfm::DAIDALUS::GroundHeadingInterval> pTempPtr (new larcfm::DAIDALUS::GroundHeadingInterval);
                        larcfm::Interval iv = m_daa_bands.track(ii,"deg");
                        double lower_trk_deg = iv.low;
                        double upper_trk_deg = iv.up;
                        larcfm::BandsRegion::Region regionType = m_daa_bands.trackRegion(ii);
                        if (regionType == larcfm::BandsRegion::FAR || regionType == larcfm::BandsRegion::MID || regionType == larcfm::BandsRegion::NEAR)
                        {
                            pTempPtr->getGroundHeadings()[0] = lower_trk_deg;
                            pTempPtr->getGroundHeadings()[1] = upper_trk_deg;
                            nogo_ptr->getWCVGroundHeadingIntervals().push_back(pTempPtr.release());
                        }
                    }
                    for (int ii = 0; ii < m_daa_bands.groundSpeedLength();++ii)
                    {
                        std::unique_ptr<larcfm::DAIDALUS::GroundSpeedInterval> pTempPtr (new larcfm::DAIDALUS::GroundSpeedInterval);
                        larcfm::Interval iv = m_daa_bands.groundSpeed(ii, "mps");
                        double lower_gs_mps = iv.low;
                        double upper_gs_mps =iv.up;
                        larcfm::BandsRegion::Region regionType = m_daa_bands.groundSpeedRegion(ii);
                        if (regionType == larcfm::BandsRegion::FAR || regionType == larcfm::BandsRegion::MID || regionType == larcfm::BandsRegion::NEAR)
                        {
                            pTempPtr->getGroundSpeeds()[0] = lower_gs_mps;
                            pTempPtr->getGroundSpeeds()[1] = upper_gs_mps;
                            nogo_ptr->getWCVGroundSpeedIntervals().push_back(pTempPtr.release());
                        }
                    }
                    for (int ii =0; ii < m_daa_bands.verticalSpeedLength();++ii)
                    {
                        std::unique_ptr<larcfm::DAIDALUS::VerticalSpeedInterval> pTempPtr (new larcfm::DAIDALUS::VerticalSpeedInterval);
                        larcfm::Interval iv = m_daa_bands.verticalSpeed(ii, "mps");
                        double lower_vs_mps = iv.low;
                        double upper_vs_mps = iv.up;
                       larcfm::BandsRegion::Region regionType = m_daa_bands.verticalSpeedRegion(ii);
                        if (regionType == larcfm::BandsRegion::FAR || regionType == larcfm::BandsRegion::MID || regionType == larcfm::BandsRegion::NEAR)
                        {
                            pTempPtr->getVerticalSpeeds()[0] = lower_vs_mps;
                            pTempPtr->getVerticalSpeeds()[1] = upper_vs_mps;
                            nogo_ptr->getWCVVerticalSpeedIntervals().push_back(pTempPtr.release());
                        }
                    }
                    for (int ii = 0; ii < m_daa_bands.altitudeLength(); ++ii)
                    {
                        std::unique_ptr<larcfm::DAIDALUS::AltitudeInterval> pTempPtr (new larcfm::DAIDALUS::AltitudeInterval);
                        larcfm::Interval iv = m_daa_bands.altitude(ii, "m");
                        double lower_alt_m = iv.low;
                        double upper_alt_m = iv.up;
                        larcfm::BandsRegion::Region regionType = m_daa_bands.altitudeRegion(ii);
                        if (regionType == larcfm::BandsRegion::FAR || regionType == larcfm::BandsRegion::MID || regionType == larcfm::BandsRegion::NEAR)
                        {
                            pTempPtr->getAltitude()[0] = lower_alt_m;
                            pTempPtr->getAltitude()[1] = upper_alt_m;
                            nogo_ptr->getWCVAlitudeIntervals().push_back(pTempPtr.release());
                        }
                    }
                    for (auto itViolations = detectedViolations.cbegin(); itViolations != detectedViolations.cend(); itViolations++)
                    {
                        std::cout << "Entity " << m_entityId << " will violate the well clear volume with Entity " << itViolations->first << " in " 
                                << itViolations->second <<" seconds!!" << std::endl<<std::endl; //--TODO delete
                       // std::cout << m_nogo_trk_deg <<  std::endl;--TODO delete
                    }
                    sendSharedLmcpObjectBroadcastMessage(nogo_ptr);
                }
                else 
                {
                    std::cout << "No violation of well clear volume detected :^)" << std::endl; //--TODO delete
                    //--TODO figure out what the appropriate action should be when there is no violation detected
                }
            }
        }
    }
    return false;
}

} //namespace service
} //namespace uxas