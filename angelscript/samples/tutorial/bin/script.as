float calc(float a, float b)
{
	Vec3d v1(1,2,3);
	Vec3d v2(2,3,4);
	Print("The Vector: " + (v1 + v2).ToString() + "\n");

    // Print the value that we received
    Print("Received: " + a + ", " + b + "\n");
    
    // Print the current system time
    Print("System has been running for " + GetSystemTime()/1000.0 + " seconds\n");
    
    // Do the calculation and return the value to the application
    return a * b;
}
