#include <gtest/gtest.h>

#ifdef ATG_ENGINE_SIM_PIRANHA_ENABLED

#include "../scripting/include/compiler.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace {
    // Mirrors EngineSimApplication::autoWireEngineScript()'s wrapper trick:
    // these asset files define `public node main { ... }` but never
    // instantiate it themselves (that's normally main.mr's job), so a
    // wrapper that imports the file and calls main() is needed to actually
    // run it.
    bool compileMainNode(const std::string &assetRelativePath, es_script::Compiler::Output *out) {
        const std::string path = std::string(ATG_ENGINE_SIM_ASSET_DIR) + "/" + assetRelativePath;
        const std::string wrapperPath = path + ".__test_wrapper.mr";

        {
            std::ofstream wrapper(wrapperPath);
            wrapper << "import \"" << path << "\"\n" << "main()\n";
        }

        es_script::Compiler compiler;
        compiler.initialize();
        const bool compiled = compiler.compile(wrapperPath);
        std::remove(wrapperPath.c_str());

        if (!compiled) {
            compiler.destroy();
            return false;
        }

        *out = compiler.execute();
        compiler.destroy();
        return true;
    }
}

TEST(MrAssetSmokeTest, ToyotaCorolla2zrfaeCvt) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/toyota/cvt/corolla_2zrfae_cvt.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::CVT);
    EXPECT_TRUE(out.transmission->allowsManualShift());
    EXPECT_FALSE(out.transmission->hasSportMode()) << "not every CVT gets an S mode";
}

// soli_dstate batch -- from-scratch engines, see assets/engines/soli_dstate/.

TEST(MrAssetSmokeTest, Toyota2jzgte) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/toyota/mt/2jzgte.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, NissanRb26dett) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/nissan/mt/rb26dett.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, NissanRb25det) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/nissan/mt/rb25det.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, BmwS65b44Mt) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/mt/S65B44_mt.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, BmwS65b44Dct) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/dct/S65B44_dct.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::DualClutch);
}

TEST(MrAssetSmokeTest, BmwS85b50Mt) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/mt/S85B50_mt.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, BmwS85b50Smg) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/amt/S85B50_smg.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::AutomatedManual);
}

TEST(MrAssetSmokeTest, BmwS55b30Mt) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/mt/S55B30_mt.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, BmwS55b30Dct) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/dct/S55B30_dct.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::DualClutch);
}

TEST(MrAssetSmokeTest, BmwB58b30m1) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/bmw/at/B58B30M1.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::AutomaticTorqueConverter);
}

TEST(MrAssetSmokeTest, HondaK20a) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/honda/mt/k20a.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, HondaK24a) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/honda/mt/k24a.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, VespaCvt) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/vespa/vespa_cvt.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::CVT);
}

TEST(MrAssetSmokeTest, FerrariF129) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/ferrari/amt/f129.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::AutomatedManual);
}

TEST(MrAssetSmokeTest, VolkswagenVr6GolfMk5) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/volkswagen/vr6_golf_mk5.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::DualClutch);
}

TEST(MrAssetSmokeTest, Lexus1lrGue) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/lexus/amt/1lr_gue.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::AutomatedManual);
}

TEST(MrAssetSmokeTest, FordCoyoteGen4Mt) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/ford/mt/coyote_gen4_mt.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

TEST(MrAssetSmokeTest, FordCoyoteGen410at) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/ford/at/coyote_gen4_10at.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::AutomaticTorqueConverter);
}

TEST(MrAssetSmokeTest, DodgeViperV10Gen5) {
    es_script::Compiler::Output out;
    ASSERT_TRUE(compileMainNode("soli_dstate/dodge/mt/viper_v10_gen5.mr", &out));
    EXPECT_NE(out.engine, nullptr);
    EXPECT_NE(out.vehicle, nullptr);
    ASSERT_NE(out.transmission, nullptr);
    EXPECT_EQ(out.transmission->getType(), Transmission::Type::Manual);
}

#endif /* ATG_ENGINE_SIM_PIRANHA_ENABLED */
