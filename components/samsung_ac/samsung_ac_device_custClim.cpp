#include "samsung_ac_device_custClim.h"
#include "esphome/core/log.h"
#include "samsung_ac.h"
#include "util.h"
#include "conversions.h"
#include <vector>
#include <set>
#include <algorithm>

namespace esphome
{
  namespace samsung_ac
  {

    climate::ClimateTraits Samsung_AC_CustClim::traits() {
      auto traits = climate::ClimateTraits();
      traits.set_supports_current_temperature(true);
      traits.set_visual_temperature_step(1);
      traits.set_visual_min_temperature(setMin);
      traits.set_visual_max_temperature(setMax);

      // --- supported modes (usiamo ClimateModeMask invece di std::set) ---
      climate::ClimateModeMask modes_mask;
      for (int i = 0; i < 7; i++) {
        if (m[i] >= 0) modes_mask.add((climate::ClimateMode)i);
      }
      traits.set_supported_modes(modes_mask);

      // --- supported presets (usiamo ClimatePresetMask invece di std::set) ---
      climate::ClimatePresetMask presets_mask;
      for (int i = 0; i < 8; i++) {
        if (p[i] >= 0) presets_mask.add((climate::ClimatePreset)i);
      }
      traits.set_supported_presets(presets_mask);

      return traits;
    }
    
    void Samsung_AC_CustClim::control(const climate::ClimateCall &call){
      ProtocolRequest request;
      auto targetTempOpt = call.get_target_temperature();
      if (targetTempOpt.has_value()) {
        request.target_temp = targetTempOpt.value();
        ESP_LOGV(TAG, "CustClim req: temp=%f", targetTempOpt.value());
      }

      auto modeOpt = call.get_mode();
      if (modeOpt.has_value()){
        request.power = modeOpt.value() > 0;
        ESP_LOGV(TAG, "CustClim req: power=%u", request.power.value());
        if (modeAddr) {
          esphome::climate::ClimateMode mod = modeOpt.value();
          if (mod < 0 || mod > 6) {
            ESP_LOGE(TAG, "Mode out of range");
          } else if (m[mod] == -1) {
            ESP_LOGE(TAG, "Mode %u recived from HA but not set in yaml", mod);
          } else {
            request.mode = (Mode)m[mod];
            ESP_LOGV(TAG, "CustClim req: mode=%u", request.mode.value());
          }
        }
      }


      auto preOpt = call.get_preset(); // preset_
      if (preOpt.has_value()){
        if (presAddr) {
          esphome::climate::ClimatePreset pres = preOpt.value();
          ESP_LOGV(TAG, "CustClim req: preset=%u", pres);

          if (pres < 0 || pres > 7) {
            ESP_LOGE(TAG, "Pres out of range");
          } else if (p[pres] < 0) {
            ESP_LOGE(TAG, "Pres %u recived from HA but not set in yaml", pres);
          } else {
            presToSend = p[pres];
            ESP_LOGV(TAG, "CustClim req: preset=%u > %i", pres, presToSend);
          }
        }
      }      

      request.caller = this;
      device->publish_request(request);
    }

    void Samsung_AC_CustClim::publishMode(){
      bool toPublish = false;

      if (!modeAddr) lastReadMode = lastEnabled; // 0-1
      if (!lastEnabled) {
        mode = esphome::climate::ClimateMode::CLIMATE_MODE_OFF;
        toPublish = true;
      } else {
        int ret = -1;
        for (int i  = 0; i < 7; i++) {
          if(m[i] == lastReadMode) ret = i; //reverse lastReadMode
        }
        if (ret == -1) {
          ESP_LOGE(TAG, "Unable to find read mode %u, not mentioned in the yaml", lastReadMode);
        } else {
          mode = (esphome::climate::ClimateMode)ret;
          ESP_LOGV(TAG, "CC changed mode, read %u>%u for addr 0x%x", lastReadMode, ret, modeAddr);
          toPublish = true;
        }
      }

      if (lastReadPres >= 0) {
        int ret = -1;
        for (int i  = 0; i < 8; i++) {
          if(p[i] == lastReadPres) ret = i; //reverse lastReadMode
        }
        if (ret == -1) {
          ESP_LOGE(TAG, "Unable to find read preset %u, not mentioned in the yaml", lastReadMode);
        } else {
          preset = (esphome::climate::ClimatePreset)ret;
          ESP_LOGV(TAG, "CC changed preset, read %u>%u", lastReadMode, ret);
          toPublish = true;
        }
      }

      if (toPublish) publish_state();
    } 

  }
  
}
