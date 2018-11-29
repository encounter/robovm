/*
 * Copyright (C) 2012 RoboVM AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>.
 */
package org.robovm.compiler.config;

import org.robovm.llvm.Target;

/**
 * @author niklas
 */
public enum OS {
    linux("linux", "linux", "linux", Family.linux),
    macosx("macosx", "macosx10.9.0", "10.9", Family.darwin),
    ios("ios", "ios7.0.0", "7.0", Family.darwin),
    horizon("switch", "horizon", "horizon", Family.horizon);

    public enum Family {linux, darwin, horizon}

    private String vmTargetName;
    private final String llvmName;
    private final String minVersion;
    private Family family;

    OS(String vmTargetName, String llvmName, String minVersion, Family family) {
        this.vmTargetName = vmTargetName;
        this.llvmName = llvmName;
        this.minVersion = minVersion;
        this.family = family;
    }

    public String getVmTargetName() {
        return vmTargetName;
    }

    public String getLlvmName() {
        return llvmName;
    }

    public String getMinVersion() {
        return minVersion;
    }

    public Family getFamily() {
        return family;
    }

    public static OS getDefaultOS() {
        String hostTriple = Target.getHostTriple();
        if (hostTriple.contains("linux")) {
            return OS.linux;
        }
        if (hostTriple.contains("darwin") || hostTriple.contains("apple")) {
            return OS.macosx;
        }
        throw new IllegalArgumentException("Unrecognized OS in host triple: " + hostTriple);
    }
}
