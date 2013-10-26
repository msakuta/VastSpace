float calc(float a, float b)
{
	Print("\n ====== Vec3d class method tests ====== \n");

	Vec3d v1(1,2,3);
	Vec3d v2(2,3,4);

	Print("\n --- Construction and string representation --- \n");
	Print("The Vector: v1 = " + v1.ToString() + ", v2 = " + v2.ToString() + "\n");

	Print("\n --- Member access --- \n");
	Print("v1[0] = " + v1[0] + ", v1[1] = " + v1[1] + "\n");
	Print("v1[0] = 1.5 = " + (v1[0] = 1.5) + "\n");
	Print("After member assignment: v1 = " + v1 + " \n");

	Print("\n --- Add and subtract operation --- \n");
	Print("Addition: v1 + v2 = " + (v1 + v2).ToString() + "\n");
	Print("Subtract: " + v1.ToString() + " - " + v2.ToString() + " = " + (v1 - v2).ToString() + "\n");

	Print("\n --- Scaling --- \n");
	Print("Scale: v1 * 36.5 = " + (v1 * 36.5).ToString() + "\n");
	Print("Scale: 27.3 * v1 = " + (27.3 * v1).ToString() + "\n");
	Print("Inverse scale: v1 / 36.5 = " + (v1 / 36.5).ToString() + "\n");

	Print("\n --- Compound operators --- \n");
	Print("Compound addition: v1 += v2 = " + (v1 += v2).ToString() + "\n");
	Print("After compound addition: v1 = " + v1.ToString() + " \n");
	Print("Compound subtraction: v1 -= v2 = " + (v1 -= v2).ToString() + "\n");
	Print("After compound subtraction: v1 = " + v1.ToString() + " \n");
	Print("Compound scale: v1 *= 36.5 = " + (v1 *= 36.5).ToString() + "\n");
	Print("After compound scale: v1 = " + v1 + "\n");
	Print("Compound inverse scale: v1 /= 36.5 = " + (v1 /= 36.5) + "\n");
	Print("After inverse compound scale: v1 = " + v1 + "\n");

	Print("\n --- Comparison --- \n");
	Print("Equality: v1 == v2 = " + (v1 == v2) + "\n");
	Print("Equality: v1 != v2 = " + (v1 != v2) + "\n");
	Print("Assignment: v1 = v2 = " + (v2 = v1) + "\n");
	Print("Equality: v1 == v2 = " + (v1 == v2) + "\n");
	Print("Equality: v1 != v2 = " + (v1 != v2) + "\n");

	Print("\n --- Unary operations --- \n");
	Print("Length: v1.len() = " + v1.len() + "\n");
	Print("Squared Length: v1.slen() = " + v1.slen() + "\n");
	Print("Normalize: v1.norm() = " + v1.norm() + "\n");
	Print("Normalize: v1.normin() = " + v1.normin() + "\n");
	Print("After normalize: v1 = " + v1 + "\n");

	Print("\n --- Binary operations --- \n");
	Print("Scalar product: v1.sp(v2) = " + v1.sp(v2) + "\n");
	Print("Vector product: v1.vp(v2) = " + v1.vp(v2) + "\n");

	Print("\n --- Strings --- \n");
	string ds("A string test\n");
	Print(ds);

    // Print the value that we received
    Print("Received: " + a + ", " + b + "\n");
    
    // Print the current system time
    Print("System has been running for " + GetSystemTime()/1000.0 + " seconds\n");
    
    // Do the calculation and return the value to the application
    return a * b;
}
