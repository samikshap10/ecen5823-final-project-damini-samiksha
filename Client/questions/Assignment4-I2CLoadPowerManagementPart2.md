Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

*Be sure to take measurements with logging disabled to ensure your logging logic is not impacting current/time measurements.*

*Please include screenshots of the profiler window detailing each current measurement captured.  See the file Instructions to add screenshots in assignment.docx in the ECEN 5823 Student Public Folder.* 

1. What is the average current per period? (Remember, once you have measured your average current, average current is average current over all time. Average current doesn’t carry with it the units of the timespan over which it was measured).
   
   Answer:
   20.64uA
   <br>Screenshot:  
   ![Avg_current_per_period](screenshots/AVG_Current_Per_Period.png)  

2. What is the ave current from the time we sleep the MCU to EM3 until we power-on the 7021 in response to the LETIMER0 UF IRQ?
  
   Answer:
   2.54uA
   <br>Screenshot:  
   ![Avg_current_LPM_Off](screenshots/Current_OFF.png)  

3. What is the ave current from the time we power-on the 7021 until we get the COMP1 IRQ indicating that the 7021's maximum time for conversion (measurement) has expired.
   
   Answer:
   232.14uA
   <br>Screenshot:  
   ![Avg_current_LPM_Off](screenshots/SENSOR_COMP1.png)  

4. How long is the Si7021 Powered On for 1 temperature reading?
   
   Answer:
   107.50ms
   <br>Screenshot:  
   ![duration_lpm_on](screenshots/ON_PERIOD_TIME.png)  

5. Given the average current per period from Q1, calculate the operating time of the system for a 1000mAh battery? - ignoring battery manufacturers, efficiencies and battery derating - just a first-order calculation.
   
Answer (in hours):

Average current consumption Iavg = 23.35uA = 0.02335mA
Battery capacity = 1000 mAh

Battery/Current = 1000mAh/23.35uA = 42821.2 hours

6. How has the power consumption performance of your design changed since the previous assignment?
   Answer:
   
The power consumption performance of my design has significantly improved compared to the previous assignment. Below are the key improvements observed:


A. Average Current Per Period:

Previous: 163.82 µA
Current: 20.64 µA
Improvement: 87.4% reduction in average current consumption.

B. Sensor Power-On Current (From enabling Si7021 until measurement completion):

Previous: 4.41 mA
Current: 520uA
Improvement: Indicating a substantial reduction in sensor power consumption.

The reduction in average current per period and sensor power-on current has drastically increased the system’s efficiency, resulting in an extended battery life. The design improvements have successfully reduced power consumption without compromising the system’s functionality.
