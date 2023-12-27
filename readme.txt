This readme explains how to set up Houdini to use the City Sample Source.

Prerequisites:
- At least 2gb of free space to extract the data and generate small cities.
- Use Houdini 18.5.532 as it is the version used for the creation of the demo.
- Run Houdini once to make sure it generates the startup files needed for the following steps.

1)
Create a new folder on your computer where the City Sample Source will be extracted.
For the following steps let's assume that the new created folder is: D:\CitySampleSource

2)
Extract the content of the CitySample_HoudiniFiles.zip file into the new folder created at step 1.
Once extracted, you should have a "Small_City" folder like so: D:\CitySampleSource\Small_City

3)
Open the "houdini.env" file located in your Windows document folder.
Example: C:\Users\{your_user_name}\Documents\houdini18.5\houdini.env

4)
At the bottom of the "houdini.env" file, add a new line and copy-paste the following line in.
HOUDINI_PATH = D:/CitySampleSource/Small_City/houdini;&
^^^^^^^^^^^^^^ copy the previous line ^^^^^^^^^^^^^^^^^

5)
Make sure that you replace the "D:/CitySampleSource" part of the path with the actual
location where you extracted the Small_City folder if it is different. Also VERY important
to use forward slashes "/" and not Windows default back slashes "\" in the "houdini.env" file.

6)
Save the "houdini.env" file and start Houdini.
If everything is good, you should be able to find the City Sample operators in
Houdini's Tab menu when inside a Geometry node. Look under the Epic Games sub-menu.

7)
You can now open the Houdini HIP file and explore the graph.
D:\CitySampleSource\Small_City\Small_City.hip
