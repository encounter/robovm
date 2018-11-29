package org.robovm.compiler.target.ios;

import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.List;

import org.robovm.compiler.config.Arch;
import org.robovm.compiler.config.Config;
import org.robovm.compiler.config.OS;
import org.robovm.compiler.target.AbstractTarget;

/**
 * SwitchTarget.
 */
public class SwitchTarget extends AbstractTarget {

    public static final String TYPE = "switch";

    private OS os;
    private Arch arch;

    public void init(Config config) {
        super.init(config);
        os = config.getOs();
        if (os == null) {
            os = OS.horizon;
        }
        arch = config.getArch();
        if (arch == null) {
            arch = Arch.arm64;
        }
    }

    @Override
    public String getType() {
        return TYPE;
    }

    @Override
    public OS getOs() {
        return os;
    }

    @Override
    public Arch getArch() {
        return arch;
    }

    @Override
    public List<Arch> getDefaultArchs() {
        return Collections.singletonList(Arch.arm64);
    }

    @Override
    public boolean canLaunch() {
        return false;
    }

    @Override
    public boolean canLaunchInPlace() {
        return false;
    }

    @Override
    protected void doBuild(File outFile, List<String> ccArgs, List<File> objectFiles, List<String> libs) throws IOException {
        // FIXME
        libs.add(0, "-L/opt/devkitpro/portlibs/switch/lib");
        libs.add(0, "-L/opt/devkitpro/libnx/lib");
        super.doBuild(outFile, ccArgs, objectFiles, libs);
    }

    @Override
    public void archive() throws IOException {
        // TODO create nro
        super.archive();
    }
}
