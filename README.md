# 2D_Grocery_students_behavior

The model includes the CO2_Source movement. Only works for 2D scenario.

Movement route:
- enter the room one by one
- walk to the area according to things they want to buy 
- stay at the location for a while
- walk to the exit, then leave the room.

# Steps
1. Compile using cmake:
    a) open the bash prompt in the Cell-DEVS-CO2_spread_computer_lab folder and execute this command, cmake ./
    b) this will create a makefile in the folder, now execute make
    c) this will create the executable co2_lab in the bin folder

2. Once compiled all changes that are not in the hpp or cpp files do not require recompilation

3. Create a folder in the bin file, name it results

4. Run the simulation by entering the bin folder, opening a bash prompt and using the command ./co2_lab followed by the path to the json and an optional number of timesteps.
      e.g ./co2_lab ../config/grocery.json    or    ./co2_lab ../config/grocery.json 500

5. The results folder will be populated with 2 files once the simulation starts running, output_messages.txt and output_state.txt
