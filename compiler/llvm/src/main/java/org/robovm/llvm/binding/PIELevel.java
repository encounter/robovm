/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.4
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.robovm.llvm.binding;

public enum PIELevel {
  PIELevelDefault(0),
  PIELevelSmall(1),
  PIELevelLarge(2);

  public final int swigValue() {
    return swigValue;
  }

  public static PIELevel swigToEnum(int swigValue) {
    PIELevel[] swigValues = PIELevel.class.getEnumConstants();
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (PIELevel swigEnum : swigValues)
      if (swigEnum.swigValue == swigValue)
        return swigEnum;
    throw new IllegalArgumentException("No enum " + PIELevel.class + " with value " + swigValue);
  }

  @SuppressWarnings("unused")
  private PIELevel() {
    this.swigValue = SwigNext.next++;
  }

  @SuppressWarnings("unused")
  private PIELevel(int swigValue) {
    this.swigValue = swigValue;
    SwigNext.next = swigValue+1;
  }

  @SuppressWarnings("unused")
  private PIELevel(PIELevel swigEnum) {
    this.swigValue = swigEnum.swigValue;
    SwigNext.next = this.swigValue+1;
  }

  private final int swigValue;

  private static class SwigNext {
    private static int next = 0;
  }
}
