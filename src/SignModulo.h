#ifndef SIGNMODULO_H
#define SIGNMODULO_H
/** \file
 * \brief Defines SignModulo and SignDiv functions. Really should be some library.
 */

/// <summary>
/// Returns remainder of divison of integers that never be negative.
/// </summary>
/// <remarks>
/// Normally, dividing two integers can result in positive or negative, depending the signs of
/// dividend and divisor.
/// In our case, this is not desirable.
/// </remarks>
/// <param name="v">Dividend</param>
/// <param name="divisor">Divisor</param>
/// <returns>Remainder</returns>
inline public int SignModulo(int v, int divisor)
{
    return (v - v / divisor * divisor + divisor) % divisor;
}

/// <summary>
/// Returns quotient of division of integers that is always greatest integer equal or less than the quotient, regardless of sign.
/// </summary>
/// <remarks>
/// Normally, dividing two integers truncates remainder of absolute value, but we want the result to be consistent regardless of
/// zero position.
/// </remarks>
/// <param name="v">Dividend</param>
/// <param name="divisor">Divisor</param>
/// <returns>Quotient</returns>
/// <seealso cref="SignModulo"/>
inline public int SignDiv(int v, int divisor)
{
    return (v - SignModulo(v, divisor)) / divisor;
}


#endif
