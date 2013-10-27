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
	Print("Assignment: v2 = v1 = " + (v2 = v1) + "\n");
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

	Print("\n ====== Quatd class method tests ====== \n");

	Quatd q1(1,2,3,4);
	Quatd q2(2,3,4,5);

	Print("\n --- Construction and string representation --- \n");
	Print("The Quaternion: q1 = " + q1.ToString() + ", q2 = " + q2.ToString() + "\n");
	Print("Construct with angle and axis: QuatdRotation(PI / 2., Vec3d(1,0,0)) = " + QuatdRotation(PI / 2., Vec3d(1,0,0)) + "\n");
	Print("Construct with direction: QuatdDirection(Vec3d(1,0,0)) = " + QuatdDirection(Vec3d(1,0,0)) + "\n");

	Print("\n --- Member access --- \n");
	Print("q1[0] = " + q1[0] + ", q1[1] = " + q1[1] + "\n");
	Print("q1[0] = 1.5 = " + (q1[0] = 1.5) + "\n");
	Print("After member assignment: q1 = " + q1 + " \n");

	Print("\n --- Add and subtract operation --- \n");
	Print("Addition: q1 + q2 = " + (q1 + q2) + "\n");
//	Print("Subtract: q1 - q2 = " + (q1 - q2) + "\n");

	Print("\n --- Compound operators --- \n");
	Print("Compound addition: q1 += q2 = " + (q1 += q2) + "\n");
	Print("After compound addition: q1 = " + q1 + " \n");

	Print("\n --- Comparison --- \n");
	Print("Equality: q1 == q2 = " + (q1 == q2) + "\n");
	Print("Equality: q1 != q2 = " + (q1 != q2) + "\n");
	Print("Assignment: q2 = q1 = " + (q2 = q1) + "\n");
	Print("Equality: q1 == q2 = " + (q1 == q2) + "\n");
	Print("Equality: q1 != q2 = " + (q1 != q2) + "\n");

	Print("\n --- Unary operations --- \n");
	Print("Length: q1.len() = " + q1.len() + "\n");
	Print("Squared Length: q1.slen() = " + q1.slen() + "\n");
	Print("Normalize: q1.norm() = " + q1.norm() + "\n");
	Print("Normalize: q1.normin() = " + q1.normin() + "\n");
	Print("After normalize: q1 = " + q1 + "\n");
	Print("Conjugate: q1.cnj() = " + q1.cnj() + "\n");

	Print("\n --- Binary operations --- \n");
	Print("Scale: q1.scale(12.3) = " + q1.scale(12.3) + "\n");
	Print("Scale: q1.scalein(12.3) = " + q1.scalein(12.3) + "\n");
	Print("After scalein: q1 = " + q1 + "\n");
	Print("Transform    : QuatdRotation(PI / 2., Vec3d(1,0,0)).trans(v1) = " + QuatdRotation(PI / 2., Vec3d(1,0,0)).trans(v1) + "\n");
	Print("Inverse trans: QuatdRotation(PI / 2., Vec3d(1,0,0)).itrans(v1) = " + QuatdRotation(PI / 2., Vec3d(1,0,0)).itrans(v1) + "\n");
	Print("quatrotquat: QuatdRotation(PI / 2., Vec3d(1,0,0)).quatrotquat(Vec3d(0,1,0)) = " + QuatdRotation(PI / 2., Vec3d(1,0,0)).quatrotquat(Vec3d(0,1,0)) + "\n");
	Print("Rotate: Quatd(1,0,0,0).rotate(PI / 2., Vec3d(1,0,0)) = " + Quatd(1,0,0,0).rotate(PI / 2., Vec3d(1,0,0)) + "\n");

	Print("\n --- Spherical linear interpolation --- \n");
	Print("Slerp: QuatdSlerp(Quatd(1,0,0,0), Quatd(0,1,0,0), .5) = " + QuatdSlerp(Quatd(1,0,0,0), Quatd(0,1,0,0), .5) + "\n");

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
