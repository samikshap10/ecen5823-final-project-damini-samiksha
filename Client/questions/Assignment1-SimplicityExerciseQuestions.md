Note: For all assignments and Energy Profiler measurements you’ll be taking this semester,  Peak measurements are instantaneous measurements taken at a specific point in time. In the Energy Profiler, this is accomplished by left-clicking at a location along the time axis.
Average measurements are measurements that are taken over a time-span. In the Energy Profiler, this is accomplished by left-clicking and dragging a region along the time axis.

Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
  
   Answer: 5.38mA - When only LED0 was kept ON with the GPIO pin set to StrongAlternateStrong.

   ![Screenshot](screenshots/SASLED0.png)

**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
  
   Answer: 5.37mA - When only LED0 was kept ON with the GPIO pin set to WeakAlternateWeak. 

   ![Screenshot](screenshots/WAWLED0.png)

**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, referencing the main board schematic, WSTK-Main-BRD4001A-A01-schematic.pdf or WSTK-Main-BRD4002A-A06-schematic.pdf, and AEM Accuracy in the ug279-brd4104a-user-guide.pdf. Both of these PDF files are available in the ECEN 5823 Student Public Folder in Google drive at: https://drive.google.com/drive/folders/1ACI8sUKakgpOLzwsGZkns3CQtc7r35bB?usp=sharing . Extra credit is available for this question and depends on your answer.**
  
   Answer:

   No, there isn't a meaningful difference in current. 
   The difference in drive strength between StrongAlternateStrong and WeakAlternateWeak won't significantly affect the LED current consumption because the static current of the LED is much lower than the maximum drive strength that the load can handle.

   Static current : (3.3 (Vsupply) - 1.8(Vled))/3000 = 0.5mA

   The static current is around 0.5mA, which is far less than the maximum current the drive strength can deliver.

   With WeakAlternateWeak setting, the GPIO can drive up to 1mA. With StrongAlternateStrong setting, it can drive up to 10mA. Since the current drawn by the LED is much lower (0.5mA), the drive strength setting doesn't significantly impact the power consumption of the LED.

   In this case, the only noticeable difference would be related to the slew rate or rise time of the drive signal. The WeakAlternateWeak setting may result in a slower rise time compared to the StrongAlternateStrong setting, but this difference has a minimal effect on the steady-state current draw of the LED.

   AEM Accuracy Reference: The accuracy of the current measurement tools could also account for the tiny difference seen. Measurement tools, depending on their accuracy, can show very small differences in the readings that don’t reflect any meaningful change in actual power consumption.


**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
  
   Answer: 5.13mA - When only LED0 was kept ON.

   ![Screenshot](screenshots/AvgCurrentLED0.png)


**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   
   Answer: 5.40mA - When both LED0 and LED1 were kept ON.
   
   ![Screenshot](screenshots/AvgCurrentLED0LED1.png)


