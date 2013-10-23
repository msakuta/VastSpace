float calc(float a, float b)
{
	Vec3d v(1,2,3);
	Print("The Vector: " + v.ToString() + "\n");
    // Print the value that we received
    Print("Received: " + a + ", " + b + "\n");
    
    // Print the current system time
    Print("System has been running for " + GetSystemTime()/1000.0 + " seconds\n");
    
    // Do the calculation and return the value to the application
    return a * b;
}
