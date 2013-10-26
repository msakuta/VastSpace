float calc(float a, float b)
{
	Vec3d v1(1,2,3);
	Vec3d v2(2,3,4);
	Print("The Vector: v1 = " + v1.ToString() + ", v2 = " + v2.ToString() + "\n");
	Print("Addition: v1 + v2 = " + (v1 + v2).ToString() + "\n");
	Print("Subtract: " + v1.ToString() + " - " + v2.ToString() + " = " + (v1 - v2).ToString() + "\n");
	Print("Scale: v1 * 36.5 = " + (v1 * 36.5).ToString() + "\n");
	Print("Scale: 27.3 * v1 = " + (27.3 * v1).ToString() + "\n");
	Print("Compound addition: v1 += v2 = " + (v1 += v2).ToString() + "\n");
	Print("After compound addition: v1 = " + v1.ToString() + " \n");
	Print("Compound subtraction: v1 -= v2 = " + (v1 -= v2).ToString() + "\n");
	Print("After compound subtraction: v1 = " + v1.ToString() + " \n");

	string ds("A dstring\n");

	Print("Hello");
	Print(ds);

    // Print the value that we received
    Print("Received: " + a + ", " + b + "\n");
    
    // Print the current system time
    Print("System has been running for " + GetSystemTime()/1000.0 + " seconds\n");
    
    // Do the calculation and return the value to the application
    return a * b;
}
