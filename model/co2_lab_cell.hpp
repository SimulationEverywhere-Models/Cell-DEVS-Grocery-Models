/**
 * Copyright (c) 2020, Cristina Ruiz Martin
 * ARSLab - Carleton University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
* Model developed by Hoda Khalil in Cell-DEVS CD++
* Implemented in Cadmium-cell-DEVS by Cristina Ruiz Martin
*/

#ifndef CADMIUM_CELLDEVS_CO2_CELL_HPP
#define CADMIUM_CELLDEVS_CO2_CELL_HPP

#include <cmath>
#include <nlohmann/json.hpp>
#include <cadmium/celldevs/cell/grid_cell.hpp>

using nlohmann::json;
using namespace cadmium::celldevs;
float cell_size = 25;

// Model Variables
int studentGenerateCount = 5; //Student generate speed (n count/student)

std::list<std::pair<int,int>> actionList; //List include the position for next CO2_Source movement action.
std::list<std::pair<int,std::pair<std::pair<int,char>,std::pair<int,int>>>> studentsList; //List include all CO2_Source that generated <<area to go,StudentID,state(+:Joining;-:Leaving)>,<xPosition,yPosition>>

std::list<std::pair<int,std::pair<int,int>>> d_areaList; //List include the information of exist workstations <workStation,<xPosition,yPosition>>
int d_areaInfoNum = 0; //Total number of exist workstations

std::list<std::pair<int,std::pair<int,int>>> foodsList;
int foodsInfoNum = 0;

std::list<std::pair<int,std::pair<int,int>>> drinksList;
int drinksInfoNum = 0;

int studentGenerated = 0; //Record the number of students the already generated
int counter = 0; //counter for studentGenerated

/************************************/
/******COMPLEX STATE STRUCTURE*******/
/************************************/
enum CELL_TYPE {AIR=-100, CO2_SOURCE=-200, IMPERMEABLE_STRUCTURE=-300, DOOR=-400, WINDOW=-500, VENTILATION=-600, DAILYUSE=-700, FOODS=-800, DRINKS=-900};
struct co2 {
    int counter;
    int concentration;
    CELL_TYPE type;
    co2() : counter(-1), concentration(500), type(AIR) {}  // a default constructor is required
    co2(int i_counter, int i_concentration, CELL_TYPE i_type) : counter(i_counter), concentration(i_concentration), type(i_type) {}
};
// Required for comparing states and detect any change
inline bool operator != (const co2 &x, const co2 &y) {
    return x.counter != y.counter || x.concentration != y.concentration || x.type != y.type;
}
// Required if you want to use transport delay (priority queue has to sort messages somehow)
inline bool operator < (const co2& lhs, const co2& rhs){ return true; }

// Required for printing the state of the cell
std::ostream &operator << (std::ostream &os, const co2 &x) {
    os << "<" << x.counter << "," << x.concentration << "," << x.type <<">";
    return os;
}

// Required for creating co2 objects from JSON file
void from_json(const json& j, co2 &s) {
    j.at("counter").get_to(s.counter);
    j.at("concentration").get_to(s.concentration);
    j.at("type").get_to(s.type);
}

/************************************/
/******COMPLEX CONFIG STRUCTURE******/
/************************************/
struct conc {
    float conc_increase; //CO2 generated by one person
    int base; //CO2 base level 500
    int window_conc; //CO2 level at window 400
    int vent_conc; //CO2 level at vent 300
    int resp_time;
    int totalStudents; //Total CO2_Source in the model
    // Each cell is 25cm x 25cm x 25cm = 15.626 Liters of air each
    // CO2 sources have their concentration continually increased by default by 12.16 ppm every 5 seconds.
    conc(): conc_increase(121.6), base(500), resp_time(5), window_conc(400), vent_conc(300), totalStudents(25) {}
    conc(float ci, int b, int wc, int vc, int r, int ts): conc_increase(ci), base(b), resp_time(r), window_conc(wc), vent_conc(vc), totalStudents(ts) {}
};
void from_json(const json& j, conc &c) {
    j.at("conc_increase").get_to(c.conc_increase);
    j.at("base").get_to(c.base);
    j.at("resp_time").get_to(c.resp_time);
    j.at("window_conc").get_to(c.window_conc);
    j.at("vent_conc").get_to(c.vent_conc);
    j.at("vent_conc").get_to(c.totalStudents);
}


template <typename T>
class co2_lab_cell : public grid_cell<T, co2> {
public:
    using grid_cell<T, co2, int>::simulation_clock;
    using grid_cell<T, co2, int>::state;
    using grid_cell<T, co2, int>::map;
    using grid_cell<T, co2, int>::neighbors;
    using grid_cell<T, co2, int>::cell_id;

    using config_type = conc;  // IMPORTANT FOR THE JSON   
    float concentration_increase; //// CO2 sources have their concentration continually increased
    int base; //CO2 base level
    int resp_time; //Time used to calculate the concentration inscrease
    int window_conc; //CO2 level at window
    int vent_conc; //CO2 level at cent
    int totalStudents; //Total CO2_Source in the model

 
    co2_lab_cell() : grid_cell<T, co2, int>() {
    }

    co2_lab_cell(cell_position const &cell_id, cell_unordered<int> const &neighborhood, co2 initial_state,
        cell_map<co2, int> const &map_in, std::string const &delayer_id, conc config) :
            grid_cell<T, co2>(cell_id, neighborhood, initial_state, map_in, delayer_id) {

        float volume = cell_size*cell_size*cell_size;
        concentration_increase = (19*100000)/volume;
        base = config.base;
        resp_time = config.resp_time;
        window_conc = config.window_conc;
        vent_conc = config.vent_conc;
        totalStudents = config.totalStudents;

        if(initial_state.type == DAILYUSE) {
            std::pair<int,std::pair<int,int>> d_areaInfo;
            d_areaInfo.first = d_areaInfoNum;
            d_areaInfo.second.first = cell_id[0];
            d_areaInfo.second.second = cell_id[1];
            d_areaInfoNum++;
            d_areaList.push_back(d_areaInfo);
        }

        if(initial_state.type == FOODS) {
            std::pair<int,std::pair<int,int>> foodsInfo;
            foodsInfo.first = foodsInfoNum;
            foodsInfo.second.first = cell_id[0];
            foodsInfo.second.second = cell_id[1];
            foodsInfoNum++;
            foodsList.push_back(foodsInfo);
        }

        if(initial_state.type == DRINKS) {
            std::pair<int,std::pair<int,int>> drinksInfo;
            drinksInfo.first = drinksInfoNum;
            drinksInfo.second.first = cell_id[0];
            drinksInfo.second.second = cell_id[1];
            drinksInfoNum++;
            drinksList.push_back(drinksInfo);
        }
    }

    co2 local_computation() const override {
        co2 new_state = state.current_state;
//        co2 new_state = state.neighbors_state.at(cell_id);

        std::pair<int,int> currentLocation;
        currentLocation.first = this->map.location[0];
        currentLocation.second = this->map.location[1];

        switch(state.current_state.type){
            case IMPERMEABLE_STRUCTURE: 
                new_state.concentration = 0;
                break;
            case DOOR:    
                new_state.concentration = base;
                break;
            case WINDOW:
                new_state.concentration = window_conc;
                break;
            case VENTILATION:
                new_state.concentration = vent_conc;
                break;
            case AIR:{
                int concentration = 0;
                int num_neighbors = 0;
                for(auto neighbors: state.neighbors_state) {
                    if( neighbors.second.concentration < 0){
                        assert(false && "co2 concentration cannot be negative");
                    }
                    if(neighbors.second.type != IMPERMEABLE_STRUCTURE){
                        concentration += neighbors.second.concentration;
                        num_neighbors +=1;
                    }
                }
                new_state.concentration = concentration/num_neighbors;

                //Appear CO2_Source at currentLocation
                if(std::find(actionList.begin(),actionList.end(),currentLocation) != actionList.end()){
                    //Arrange the next action
                    new_state.type = CO2_SOURCE;
                }

                if (currentLocation.first == 23 && currentLocation.second == 5) {
                    if (counter == 0 && studentGenerated < totalStudents && studentGenerated < (d_areaInfoNum+foodsInfoNum+drinksInfoNum)/2){
                        //Given student ID and record the location
                        //std::pair<std::pair<int,char>,std::pair<int,int>> studentID;
                        std::pair<int,std::pair<std::pair<int,char>,std::pair<int,int>>> studentID;
                        srand(time(0));
                        studentID.first = (rand() % 3) + 1;
                        studentID.second.first.first = studentGenerated;
                        studentID.second.first.second = '+';
                        studentID.second.second = currentLocation;
                        studentsList.push_back(studentID);

                        //Arrange the next action
                        actionList.push_back(currentLocation);

                        studentGenerated++;
                        new_state.type = CO2_SOURCE;
                    }
                    counter = (counter + 1) % studentGenerateCount;
                }
                break;
            }
            case DAILYUSE:{
                int concentration = 0;
                int num_neighbors = 0;
                for(auto neighbors: state.neighbors_state) {
                    if( neighbors.second.concentration < 0){
                        assert(false && "co2 concentration cannot be negative");
                    }
                    if(neighbors.second.type != IMPERMEABLE_STRUCTURE){
                        concentration += neighbors.second.concentration;
                        num_neighbors +=1;
                    }
                }
                new_state.concentration = concentration/num_neighbors;
                break;
            }
            case FOODS:{
                int concentration = 0;
                int num_neighbors = 0;
                for(auto neighbors: state.neighbors_state) {
                    if( neighbors.second.concentration < 0){
                        assert(false && "co2 concentration cannot be negative");
                    }
                    if(neighbors.second.type != IMPERMEABLE_STRUCTURE){
                        concentration += neighbors.second.concentration;
                        num_neighbors +=1;
                    }
                }
                new_state.concentration = concentration/num_neighbors;
                break;
            }
            case DRINKS:{
                int concentration = 0;
                int num_neighbors = 0;
                for(auto neighbors: state.neighbors_state) {
                    if( neighbors.second.concentration < 0){
                        assert(false && "co2 concentration cannot be negative");
                    }
                    if(neighbors.second.type != IMPERMEABLE_STRUCTURE){
                        concentration += neighbors.second.concentration;
                        num_neighbors +=1;
                    }
                }
                new_state.concentration = concentration/num_neighbors;
                break;
            }
            case CO2_SOURCE:{
                int concentration = 0;
                int num_neighbors = 0;
                for(auto neighbors: state.neighbors_state) {
                    if( neighbors.second.concentration < 0){
                        assert(false && "co2 concentration cannot be negative");
                    }
                    if(neighbors.second.type != IMPERMEABLE_STRUCTURE){
                        concentration += neighbors.second.concentration;
                        num_neighbors +=1;
                    }
                }

                new_state.concentration = (concentration/num_neighbors) + (concentration_increase);
                new_state.counter += 1;

                //Remove CO2_Source at currentLocation
                if(std::find(actionList.begin(),actionList.end(),currentLocation) != actionList.end()){
                    srand(time(0));
                    int randomNumber = (rand() % 60) + 60;
                    std::list<std::pair<int, std::pair<std::pair<int, char>, std::pair<int, int>>>>::iterator i;
                    for (i = studentsList.begin(); i != studentsList.end(); i++) {
                        if (i->second.second == currentLocation) { //Find the corresponding student
                            if(state.current_state.counter >= randomNumber){
                                i->second.first.second = '-';
                            }

                            std::pair< int,std::pair<int, char>> stuID;
                            stuID = std::make_pair(i->first, i->second.first);
                            std::pair<int, int> nextLocation = setNextRoute(currentLocation, stuID );
                            i->second.second = nextLocation;

                            if(nextLocation == currentLocation){ //Stay at same location
                                //DO NOTHING
                            }else if(nextLocation.first == -1 && nextLocation.second == -1){
                                //Change the type
                                new_state.type = AIR;
                                actionList.remove(currentLocation);
                            }else {
                                //Arrangement next action and change the type
                                actionList.remove(currentLocation);
                                actionList.push_back(nextLocation);
                                new_state.type = AIR;
                            }
                        }
                    }
                }
                break;
            }
            default:{
                assert(false && "should never happen");
            }
        }
        return new_state;
    }

    /*
     * Calculate the position after the movement
     *
     * return: nextLocation
     */
    [[nodiscard]] std::pair<int,int> setNextRoute(std::pair<int,int> location, std::pair<int, std::pair<int, char>> studentIDNumber) const {
        std::pair<int, int> nextLocation;
        std::pair<int, int> destination;
        std::pair<int, int> locationChange;
        int destinationWSNum;
        switch(studentIDNumber.first){
            case 1:{ 
                destinationWSNum = studentIDNumber.second.first % d_areaInfoNum;
                if(studentIDNumber.second.second == '-'){
            		destination.first = 24;
            		destination.second = 5;

            		if(doorNearby(destination)){
                		nextLocation.first = -1;
                		nextLocation.second = -1;
                		return nextLocation;
            		}
        		}
        		else {
            		//Get destination workstation location
            		for (auto const i:d_areaList) {
                		if (i.first == destinationWSNum) {
                    		destination = i.second;
                		}
            		}
            		if(WSNearby(destination)){
                		nextLocation = location;
                		return nextLocation;
            		}
        		}
                break;}
            
            case 2:{    
                destinationWSNum = studentIDNumber.second.first % foodsInfoNum;
                if(studentIDNumber.second.second == '-'){
            		destination.first = 24;
            		destination.second = 5;

            		if(doorNearby(destination)){
                		nextLocation.first = -1;
                		nextLocation.second = -1;
                		return nextLocation;
            		}
        		}
        		else {
            		//Get destination workstation location
            		for (auto const i:foodsList) {
                		if (i.first == destinationWSNum) {
                    		destination = i.second;
                		}
            		}
            		if(WSNearby(destination)){
                		nextLocation = location;
                		return nextLocation;
            		}
        		}
                break;}
            
            case 3:{
                destinationWSNum = studentIDNumber.second.first % drinksInfoNum;
                if(studentIDNumber.second.second == '-'){
            		destination.first = 24;
            		destination.second = 5;

            		if(doorNearby(destination)){
                		nextLocation.first = -1;
                		nextLocation.second = -1;
                		return nextLocation;
            		}
        		}
        		else {
            		//Get destination workstation location
            		for (auto const i:drinksList) {
                		if (i.first == destinationWSNum) {
                    		destination = i.second;
                		}
            		}
            		if(WSNearby(destination)){
                		nextLocation = location;
                		return nextLocation;
            		}
        		}
                break;}
        }
        //int destinationWSNum = studentIDNumber.first % workstationNumber;

        int x_diff = abs(location.first - destination.first);
        int y_diff = abs(location.second - destination.second);

        if(y_diff <= 2 && x_diff > 0) { // x as priority direction
            if (destination.first < location.first) { //move left
                locationChange = navigation(location,'x','-');
            }else{//move right
                locationChange = navigation(location,'x','+');
            }
        }else{ // y as priority direction
            if (destination.second < location.second) { //move up
                locationChange = navigation(location,'y','-');
            }else{//move down
                locationChange = navigation(location,'y','+');
            }
        }

        nextLocation.first = location.first + locationChange.first;
        nextLocation.second = location.second + locationChange.second;

        return nextLocation;
    }

    /*
     * Check if the destination workstation is nearby
     *
     * return true if Workstation nearby
     */
    [[nodiscard]] bool WSNearby(std::pair<int, int> destination) const {
        for(auto const neighbors: state.neighbors_state) {
            if(neighbors.second.type==DAILYUSE || neighbors.second.type==FOODS || neighbors.second.type==DRINKS) {
                if (neighbors.first[0] == destination.first) {
                    if (neighbors.first[1] == destination.second) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /*
     * Check if the destination DOOR is nearby
     *
     * return true if DOOR is nearby
     */
    [[nodiscard]] bool doorNearby(std::pair<int, int> destination) const{
        for(auto const neighbors: state.neighbors_state) {
            if(neighbors.second.type == DOOR) {
//                if (neighbors.first[0] == destination.first) {
//                    if (neighbors.first[1] == destination.second) {
                        return true;
//                    }
//                }
            }
        }
        return false;
    }

    /*
     * Using movement rules to do the navigation
     *
     * return: the location change
     */
    [[nodiscard]] std::pair<int,int> navigation(std::pair<int,int> location, char priority, char direction) const {
        std::pair<int,int> locationChange;
        locationChange.first = 0;
        locationChange.second = 0;

        int change;
        if(direction == '-'){
            change = -1;
        } else{
            change = 1;
        }

        if(priority == 'x'){
            if(moveCheck(location.first + change, location.second)){
                locationChange.first = change;
            }else if(moveCheck(location.first, location.second + change)){
                locationChange.second = change;
            }else if(moveCheck(location.first, location.second - change)){
                locationChange.second = 0 - change;
            }else if(moveCheck(location.first - change, location.second)){
                locationChange.first = change;
            }
        }else{
            if(moveCheck(location.first, location.second + change)){
                locationChange.second = change;
            }else if(moveCheck(location.first + change, location.second)){
                locationChange.first = change;
            }else if(moveCheck(location.first - change, location.second)){
                locationChange.first = 0 - change;
            }else if(moveCheck(location.first, location.second - change)){
                locationChange.second = change;
            }
        }
        return locationChange;
    }

    /*
     * Check if the next location is occupied
     *
     * return true if it's available to move
     */
    [[nodiscard]] bool moveCheck(int xNext,int yNext) const {
        bool moveCheck = false;
        for(auto const neighbors: state.neighbors_state) {
            if(neighbors.first[0] == xNext){
                if(neighbors.first[1] == yNext){
                    if(neighbors.second.type == AIR) {
                        moveCheck = true;
                    }
                }
            }
        }

        for(auto const student: studentsList){
            if(student.second.second.first == xNext){
                if(student.second.second.second == yNext){
                    moveCheck = false;
                }
            }
        }

        return moveCheck;
    }

    // It returns the delay to communicate cell's new state.
    T output_delay(co2 const &cell_state) const override {
        switch(cell_state.type){
            case CO2_SOURCE: return resp_time;
            default: return 1;
        }
    }

};

#endif //CADMIUM_CELLDEVS_CO2_CELL_HPP













