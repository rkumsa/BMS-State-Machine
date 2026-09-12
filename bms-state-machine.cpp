#include <string>
#include <iostream>
#include <cstdlib>  
#include <ctime>   


// States 
enum class State { 
    Off,
    Precharging,
    Energized,
    ReadyToDrive,
    Fault,
    Charging,
    ChargingFault,
 };

// Events
enum class Event { 
    StartSequence,
    ChargerConnected,
    PrechargeComplete,
    ChargingComplete,
    ReadyToDriveRequested,
    ExitReadyToDrive,
    ShutdownRequested, 
    CellVoltageFault,
    CellTempFault,
    FuseFault,
    MissingReading,
    InternalFault,
    ManualReset,
    None

 };


State transition(State current, Event e);

// transition states
State transition(State current, Event e) {
    switch (current) {
        case State::Off:
            switch (e) {
                case Event::StartSequence:
                    return State::Precharging;
                case Event::ChargerConnected:
                    return State::Charging;
                default:
                    return current;
            }

        case State::Precharging:
            switch (e) {
                case Event::PrechargeComplete:
                    return State::Energized;
                case Event::ShutdownRequested:
                    return State::Off;
                case Event::CellVoltageFault:
                case Event::CellTempFault:
                case Event::FuseFault:
                case Event::MissingReading:
                case Event::InternalFault:
                    return State::Fault;
                default:
                    return current;
            }

        case State::Energized:
            switch (e) {
                case Event::ReadyToDriveRequested:
                    return State::ReadyToDrive;
                case Event::ShutdownRequested:
                    return State::Off;
                case Event::CellVoltageFault:
                case Event::CellTempFault:
                case Event::FuseFault:
                case Event::MissingReading:
                case Event::InternalFault:
                    return State::Fault;
                default:
                    return current;
            }

        case State::ReadyToDrive:
            switch (e) {
                case Event::ShutdownRequested:
                    return State::Off;
                case Event::CellVoltageFault:
                case Event::CellTempFault:
                case Event::FuseFault:
                case Event::MissingReading:
                case Event::InternalFault:
                    return State::Fault;
                case Event::ExitReadyToDrive:
                    return State::Energized;
                default:
                    return current;
            }
        case State::Charging:
            switch (e) {
                case Event::ShutdownRequested:
                    return State::Off;
                case Event::ChargingComplete:
                    return State::Off;
                case Event::CellVoltageFault:
                case Event::CellTempFault:
                case Event::FuseFault:
                case Event::MissingReading:
                case Event::InternalFault:
                    return State::ChargingFault;
                default:
                    return current;
            }
        case State::Fault:
            switch (e) {
                case Event::ManualReset:
                    return State::Off;
                default:
                    return current;
            }
        
        case State::ChargingFault:
            switch (e) {
                case Event::ManualReset:
                    return State::Off;
                default:
                    return current;
            }
    }
    return current;
}

// toString to display states in terminal
std::string toString(State s) {
    switch (s) {
        case State::Off: return "Off";
        case State::Precharging: return "Precharging";
        case State::Energized: return "Energized";
        case State::ReadyToDrive: return "ReadyToDrive";
        case State::Fault: return "Fault";
        case State::Charging: return "Charging";
        case State::ChargingFault: return "ChargingFault";
    }
    return "Unknown";
}

// input events in terminal to test code
Event checkEvent(const std::string& input) {
    if (input == "started") 
        return Event::StartSequence;
    if (input == "charger connected") 
        return Event::ChargerConnected;
    if (input == "precharging completed") 
        return Event::PrechargeComplete;
    if (input == "charging completed") 
        return Event::ChargingComplete;
    if (input == "brake pressed and button pressed") 
        return Event::ReadyToDriveRequested;
    if (input == "shutdown requested") 
        return Event::ShutdownRequested;
    if (input == "cell went over voltage") 
        return Event::CellVoltageFault;
    if (input == "cell went under voltage") 
        return Event::CellVoltageFault;
    if (input == "cell went over temp") 
        return Event::CellTempFault;
    if (input == "cell went under temp") 
        return Event::CellTempFault;
    if (input == "fuse blown/tripped") 
        return Event::FuseFault;
    if (input == "missed reading") 
        return Event::MissingReading;
    if (input == "internal bms fault occured") 
        return Event::InternalFault;
    if (input == "manually reset") 
        return Event::ManualReset;
    if(input == "brake released or button pressed") 
        return Event::ExitReadyToDrive;
    return Event::None;
}

//CAN STUFF BELOW

bool bmsIsMonitoring(State s) {
    switch (s) {
        case State::Precharging:
        case State::Energized:
        case State::ReadyToDrive:
        case State::Charging:
            return true;
        default:
            return false;
    }
}


//check voltage
Event checkVoltageMessage(double minCellVoltage, double maxCellVoltage) {
    if (minCellVoltage < 2.50 || maxCellVoltage > 4.20) {
        return Event::CellVoltageFault;
    }
    return Event::None;
}

//check temp
Event checkTempMessage(double minCellTemp, double maxCellTemp, State curr) {

    if(curr == State::Charging) {
        if (minCellTemp < 0 || maxCellTemp > 45) {
            return Event::CellTempFault;
        }
    }
    else {
            if (minCellTemp < -20 || maxCellTemp > 60) {
                return Event::CellTempFault;
            }
    }

    return Event::None;
}

//generate random voltages/temps
double randomInRange(double low, double high) {
    return low + (high - low) * (rand() / (double)RAND_MAX);
}

//simulate can voltages
void runVoltageTest(State& current, int numReadings) {
    double minVol = 999.9;
    double maxVol = -999.9;

    for (int i = 1; i <= numReadings; ++i) {
        double reading = randomInRange(2.0, 4.5);
        if (reading < minVol) minVol = reading;
        if (reading > maxVol) maxVol = reading;

        if (i % 10 == 0) {
            Event e = checkVoltageMessage(minVol, maxVol);
            current = transition(current, e);
            std::cout << "  [reading " << i << "] min=" << minVol
                      << " max=" << maxVol
                      << " -> State: " << toString(current) << std::endl;

            minVol = 999.9;
            maxVol = -999.9;
        }
    }
}

//simulate can temps
void runTempTest(State& current, int numReadings) {
    double minTemp = 999.9;
    double maxTemp = -999.9;

    for (int i = 1; i <= numReadings; ++i) {
        double reading = randomInRange(-25.0, 65.0);
        if (reading < minTemp) {
            minTemp = reading;
        }
        if (reading > maxTemp) {
            maxTemp = reading;
        }

        if (i % 10 == 0) {
            Event e = checkTempMessage(minTemp, maxTemp, current);
            current = transition(current, e);
            std::cout << "  [reading " << i << "] min=" << minTemp
                      << " max=" << maxTemp
                      << " -> State: " << toString(current) << std::endl;

            minTemp = 999.9;
            maxTemp = -999.9;
        }
    }
}


//main function
int main() {
    State current = State::Off;
    std::string line;
    srand(time(nullptr));

    std::cout << "State: " << toString(current) << std::endl;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

    if (line == "test can voltages") {
        if (!bmsIsMonitoring(current)) {
            std::cout << "BMS not monitoring — Tractive System not active and not charging (EV.7.3.1)"  << std::endl;
        } 
        else {
            runVoltageTest(current, 100);
        }
        continue;
    }
    if (line == "test can temps") {
        if (!bmsIsMonitoring(current)) {
            std::cout << "BMS not monitoring — Tractive System not active and not charging (EV.7.3.1)"   << std::endl;
        } 
        else {
            runTempTest(current, 100);
        }
        continue;
    }
        Event e = checkEvent(line);
        if (e == Event::None) {
            std::cout << "unrecognized input" << std::endl;
            continue;
        }
        current = transition(current, e);
        std::cout << "State: " << toString(current) << std::endl;
    }
    return 0;
}