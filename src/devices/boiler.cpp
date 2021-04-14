/*
 * EMS-ESP - https://github.com/emsesp/EMS-ESP
 * Copyright 2020  Paul Derbyshire
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "boiler.h"

namespace emsesp {

REGISTER_FACTORY(Boiler, EMSdevice::DeviceType::BOILER)

uuid::log::Logger Boiler::logger_{F_(boiler), uuid::log::Facility::CONSOLE};

Boiler::Boiler(uint8_t device_type, int8_t device_id, uint8_t product_id, const std::string & version, const std::string & name, uint8_t flags, uint8_t brand)
    : EMSdevice(device_type, device_id, product_id, version, name, flags, brand) {
    // register values only for master boiler/cascade module
    if (device_id != EMSdevice::EMS_DEVICE_ID_BOILER) {
        return;
    }

    reserve_mem(20); // reserve some space for the telegram registries, to avoid memory fragmentation

    LOG_DEBUG(F("Adding new Boiler with device ID 0x%02X"), device_id);

    // the telegram handlers...
    register_telegram_type(0x10, F("UBAErrorMessage1"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAErrorMessage(t); });
    register_telegram_type(0x11, F("UBAErrorMessage2"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAErrorMessage(t); });
    register_telegram_type(0x14, F("UBATotalUptime"), true, [&](std::shared_ptr<const Telegram> t) { process_UBATotalUptime(t); });
    register_telegram_type(0x15, F("UBAMaintenanceData"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMaintenanceData(t); });
    register_telegram_type(0x16, F("UBAParameters"), true, [&](std::shared_ptr<const Telegram> t) { process_UBAParameters(t); });
    register_telegram_type(0x18, F("UBAMonitorFast"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMonitorFast(t); });
    register_telegram_type(0x19, F("UBAMonitorSlow"), true, [&](std::shared_ptr<const Telegram> t) { process_UBAMonitorSlow(t); });
    register_telegram_type(0x1A, F("UBASetPoints"), false, [&](std::shared_ptr<const Telegram> t) { process_UBASetPoints(t); });
    register_telegram_type(0x1C, F("UBAMaintenanceStatus"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMaintenanceStatus(t); });
    register_telegram_type(0x2A, F("MC10Status"), false, [&](std::shared_ptr<const Telegram> t) { process_MC10Status(t); });
    register_telegram_type(0x33, F("UBAParameterWW"), true, [&](std::shared_ptr<const Telegram> t) { process_UBAParameterWW(t); });
    register_telegram_type(0x34, F("UBAMonitorWW"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMonitorWW(t); });
    register_telegram_type(0x35, F("UBAFlags"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAFlags(t); });
    register_telegram_type(0xD1, F("UBAOutdoorTemp"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAOutdoorTemp(t); });
    register_telegram_type(0xE3, F("UBAMonitorSlowPlus"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMonitorSlowPlus2(t); });
    register_telegram_type(0xE4, F("UBAMonitorFastPlus"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMonitorFastPlus(t); });
    register_telegram_type(0xE5, F("UBAMonitorSlowPlus"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAMonitorSlowPlus(t); });
    register_telegram_type(0xE6, F("UBAParametersPlus"), true, [&](std::shared_ptr<const Telegram> t) { process_UBAParametersPlus(t); });
    register_telegram_type(0xE9, F("UBADHWStatus"), false, [&](std::shared_ptr<const Telegram> t) { process_UBADHWStatus(t); });
    register_telegram_type(0xEA, F("UBAParameterWWPlus"), true, [&](std::shared_ptr<const Telegram> t) { process_UBAParameterWWPlus(t); });
    register_telegram_type(0x494, F("UBAEnergySupplied"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAEnergySupplied(t); });
    register_telegram_type(0x495, F("UBAInformation"), false, [&](std::shared_ptr<const Telegram> t) { process_UBAInformation(t); });

    // MQTT commands for boiler topic
    register_mqtt_cmd(F("comfort"), [&](const char * value, const int8_t id) { return set_warmwater_mode(value, id); });
    register_mqtt_cmd(F("wwactivated"), [&](const char * value, const int8_t id) { return set_warmwater_activated(value, id); });
    register_mqtt_cmd(F("wwtapactivated"), [&](const char * value, const int8_t id) { return set_tapwarmwater_activated(value, id); });
    register_mqtt_cmd(F("wwonetime"), [&](const char * value, const int8_t id) { return set_warmwater_onetime(value, id); });
    register_mqtt_cmd(F("wwcircpump"), [&](const char * value, const int8_t id) { return set_warmwater_circulation_pump(value, id); });
    register_mqtt_cmd(F("wwcirculation"), [&](const char * value, const int8_t id) { return set_warmwater_circulation(value, id); });
    register_mqtt_cmd(F("wwcircmode"), [&](const char * value, const int8_t id) { return set_warmwater_circulation_mode(value, id); });
    register_mqtt_cmd(F("flowtemp"), [&](const char * value, const int8_t id) { return set_flow_temp(value, id); });
    register_mqtt_cmd(F("wwsettemp"), [&](const char * value, const int8_t id) { return set_warmwater_temp(value, id); });
    register_mqtt_cmd(F("heatingactivated"), [&](const char * value, const int8_t id) { return set_heating_activated(value, id); });
    register_mqtt_cmd(F("heatingtemp"), [&](const char * value, const int8_t id) { return set_heating_temp(value, id); });
    register_mqtt_cmd(F("burnmaxpower"), [&](const char * value, const int8_t id) { return set_max_power(value, id); });
    register_mqtt_cmd(F("burnminpower"), [&](const char * value, const int8_t id) { return set_min_power(value, id); });
    register_mqtt_cmd(F("boilhyston"), [&](const char * value, const int8_t id) { return set_hyst_on(value, id); });
    register_mqtt_cmd(F("boilhystoff"), [&](const char * value, const int8_t id) { return set_hyst_off(value, id); });
    register_mqtt_cmd(F("burnperiod"), [&](const char * value, const int8_t id) { return set_burn_period(value, id); });
    register_mqtt_cmd(F("pumpdelay"), [&](const char * value, const int8_t id) { return set_pump_delay(value, id); });
    // register_mqtt_cmd(F("reset"), [&](const char * value, const int8_t id) { return set_reset(value, id); });
    register_mqtt_cmd(F("maintenance"), [&](const char * value, const int8_t id) { return set_maintenance(value, id); });
    register_mqtt_cmd(F("pumpmodmax"), [&](const char * value, const int8_t id) { return set_max_pump(value, id); });
    register_mqtt_cmd(F("pumpmodmin"), [&](const char * value, const int8_t id) { return set_min_pump(value, id); });

    EMSESP::send_read_request(0x10, device_id); // read last errorcode on start (only published on errors)
    EMSESP::send_read_request(0x11, device_id); // read last errorcode on start (only published on errors)
    EMSESP::send_read_request(0x15, device_id); // read maintenace data on start (only published on change)
    EMSESP::send_read_request(0x1C, device_id); // read maintenace status on start (only published on change)
}

// create the config topics for Home Assistant MQTT Discovery
// for each of the main elements
void Boiler::register_mqtt_ha_config() {
    if (!Mqtt::connected()) {
        return;
    }

    // Create the Master device
    StaticJsonDocument<EMSESP_MAX_JSON_SIZE_HA_CONFIG> doc;
    doc["name"]    = FJSON("Service Code");
    doc["uniq_id"] = FJSON("boiler");
    doc["ic"]      = FJSON("mdi:home-thermometer-outline");

    char stat_t[128];
    snprintf_P(stat_t, sizeof(stat_t), PSTR("%s/boiler_data"), Mqtt::base().c_str());
    doc["stat_t"] = stat_t;

    doc["val_tpl"] = FJSON("{{value_json.serviceCode}}");
    JsonObject dev = doc.createNestedObject("dev");
    dev["name"]    = FJSON("EMS-ESP Boiler");
    dev["sw"]      = EMSESP_APP_VERSION;
    dev["mf"]      = brand_to_string();
    dev["mdl"]     = name();
    JsonArray ids  = dev.createNestedArray("ids");
    ids.add("ems-esp-boiler");
    Mqtt::publish_ha(F("homeassistant/sensor/ems-esp/boiler/config"), doc.as<JsonObject>()); // publish the config payload with retain flag

    Mqtt::register_mqtt_ha_binary_sensor(F_(tapwaterActive), device_type(), "tapwater_active");
    Mqtt::register_mqtt_ha_binary_sensor(F_(heatingActive), device_type(), "heating_active");

    // main
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(serviceCodeNumber), device_type(), "serviceCodeNumber", nullptr, F_(iconpower));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(selFlowTemp), device_type(), "selFlowTemp", F_(degrees), F_(iconcruise));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(selBurnPow), device_type(), "selBurnPow", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(curBurnPow), device_type(), "curBurnPow", F_(percent), F_(iconfire));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(heatingPumpMod), device_type(), "heatingPumpMod", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(heatingPump2Mod), device_type(), "heatingPump2Mod", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(outdoorTemp), device_type(), "outdoorTemp", F_(degrees), F_(iconexport));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(curFlowTemp), device_type(), "curFlowTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(retTemp), device_type(), "retTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(mixerTemp), device_type(), "mixerTemp", F_(degrees), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(tankMiddleTemp), device_type(), "tankMiddleTemp", F_(degrees), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(switchTemp), device_type(), "switchTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(sysPress), device_type(), "sysPress", F_(bar), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(boilTemp), device_type(), "boilTemp", F_(degrees), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(burnGas), device_type(), "burnGas", nullptr, F_(iconfire));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(flameCurr), device_type(), "flameCurr", F_(uA), F_(iconflash));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(heatingPump), device_type(), "heatingPump", nullptr, F_(iconpump));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(fanWork), device_type(), "fanWork", nullptr, F_(iconfan));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(ignWork), device_type(), "ignWork", nullptr, F_(iconflash));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(exhaustTemp), device_type(), "exhaustTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(heatingActivated), device_type(), "heatingActivated", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(heatingTemp), device_type(), "heatingTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(pumpModMax), device_type(), "pumpModMax", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(pumpModMin), device_type(), "pumpModMin", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(pumpDelay), device_type(), "pumpDelay", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(burnMinPeriod), device_type(), "burnMinPeriod", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(burnMinPower), device_type(), "burnMinPower", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(burnMaxPower), device_type(), "burnMaxPower", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(boilHystOn), device_type(), "boilHystOn", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(boilHystOff), device_type(), "boilHystOff", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(setFlowTemp), device_type(), "setFlowTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(setBurnPow), device_type(), "setBurnPow", F_(percent), F_(iconpercent));
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(burnStarts), device_type(), "burnStarts", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(burnWorkMin), device_type(), "burnWorkMin", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(heatWorkMin), device_type(), "heatWorkMin", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(UBAuptime), device_type(), "UBAuptime", F_(min), nullptr);
    // optional in info
    // Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(maintenanceMessage), device_type(), "maintenanceMessage", nullptr, nullptr);
    // Mqtt::register_mqtt_ha_sensor(nullptr, nullptr, F_(maintenance), device_type(), "maintenance", nullptr, nullptr);

    // information
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(upTimeControl), device_type(), "upTimeControl", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(upTimeCompHeating), device_type(), "upTimeCompHeating", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(upTimeCompCooling), device_type(), "upTimeCompCooling", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(upTimeCompWw), device_type(), "upTimeCompWw", F_(min), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(heatingStarts), device_type(), "heatingStarts", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(coolingStarts), device_type(), "coolingStarts_", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(wWStarts2), device_type(), "wWStarts2", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgConsTotal), device_type(), "nrgConsTotal", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(auxElecHeatNrgConsTotal), device_type(), "auxElecHeatNrgConsTotal_", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(auxElecHeatNrgConsHeating), device_type(), "auxElecHeatNrgConsHeating", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(auxElecHeatNrgConsDHW), device_type(), "auxElecHeatNrgConsDHW", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgConsCompTotal), device_type(), "nrgConsCompTotal", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgConsCompHeating), device_type(), "nrgConsCompHeating", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgConsCompWw), device_type(), "nrgConsCompWw", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgConsCompCooling), device_type(), "nrgConsCompCooling", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgSuppTotal), device_type(), "nrgSuppTotal_", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgSuppHeating), device_type(), "nrgSuppHeating", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgSuppWw), device_type(), "nrgSuppWw", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(nrgSuppCooling), device_type(), "nrgSuppCooling", F_(kwh), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(maintenanceMessage), device_type(), "maintenanceMessage", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(maintenance), device_type(), "maintenance", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(maintenanceTime), device_type(), "maintenanceTime", F_(hours), nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_info), F_(maintenanceDate), device_type(), "maintenanceDate", nullptr, nullptr);

    mqtt_ha_config_ = true; // done
}

// create the config topics for Home Assistant MQTT Discovery
// for each of the ww elements
void Boiler::register_mqtt_ha_config_ww() {
    if (!Mqtt::connected()) {
        return;
    }

    // ww
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWSelTemp), device_type(), "wWSelTemp", F_(degrees), F_(iconcruise));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWSetTemp), device_type(), "wWSetTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWDisinfectionTemp), device_type(), "wWDisinfectionTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWType), device_type(), "wWType", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWChargeType), device_type(), "wWChargeType", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCircPump), device_type(), "wWCircPump", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCircPumpMode), device_type(), "wWCircPumpMode", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCirc), device_type(), "wWCirc", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCurTemp), device_type(), "wWCurTemp", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCurTemp2), device_type(), "wWCurTemp2", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCurFlow), device_type(), "wWCurFlow", F("l/min"), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWHeat), device_type(), "wWHeat", nullptr, F_(iconvalve));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWStorageTemp1), device_type(), "wWStorageTemp1", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWStorageTemp2), device_type(), "wWStorageTemp2", F_(degrees), F_(iconwatertemp));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWActivated), device_type(), "wWActivated", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWOneTime), device_type(), "wWOneTime", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWDisinfecting), device_type(), "wWDisinfecting", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWCharging), device_type(), "wWCharging", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWRecharging), device_type(), "wWRecharging", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWTempOK), device_type(), "wWTempOK", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWActive), device_type(), "wWActive", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWSetPumpPower), device_type(), "wWSetPumpPower", F_(percent), F_(iconpump));
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWStarts), device_type(), "wWStarts", nullptr, nullptr);
    Mqtt::register_mqtt_ha_sensor(nullptr, F_(mqtt_suffix_ww), F_(wWWorkM), device_type(), "wWWorkM", F_(min), nullptr);

    mqtt_ha_config_ww_ = true; // done
}

// send stuff to the Web UI
void Boiler::device_info_web(JsonArray & root, uint8_t & part) {
    // fetch the values into a JSON document
    StaticJsonDocument<EMSESP_MAX_JSON_SIZE_LARGE> doc;
    JsonObject                                     json = doc.to<JsonObject>();
    if (part == 0) {
        part = 1; // we have another part
        if (!export_values_main(json, true)) {
            return; // empty
        }

        create_value_json(root, F("heatingActive"), nullptr, F_(heatingActive), nullptr, json);
        create_value_json(root, F("tapwaterActive"), nullptr, F_(tapwaterActive), nullptr, json);
        create_value_json(root, F("serviceCode"), nullptr, F_(serviceCode), nullptr, json);
        create_value_json(root, F("serviceCodeNumber"), nullptr, F_(serviceCodeNumber), nullptr, json);
        create_value_json(root, F("lastCode"), nullptr, F_(lastCode), nullptr, json);
        create_value_json(root, F("selFlowTemp"), nullptr, F_(selFlowTemp), F_(degrees), json);
        create_value_json(root, F("selBurnPow"), nullptr, F_(selBurnPow), F_(percent), json);
        create_value_json(root, F("curBurnPow"), nullptr, F_(curBurnPow), F_(percent), json);
        create_value_json(root, F("heatingPumpMod"), nullptr, F_(heatingPumpMod), F_(percent), json);
        create_value_json(root, F("heatingPump2Mod"), nullptr, F_(heatingPump2Mod), F_(percent), json);
        create_value_json(root, F("outdoorTemp"), nullptr, F_(outdoorTemp), F_(degrees), json);
        create_value_json(root, F("curFlowTemp"), nullptr, F_(curFlowTemp), F_(degrees), json);
        create_value_json(root, F("retTemp"), nullptr, F_(retTemp), F_(degrees), json);
        create_value_json(root, F("switchTemp"), nullptr, F_(switchTemp), F_(degrees), json);
        create_value_json(root, F("mixerTemp"), nullptr, F_(mixerTemp), F_(degrees), json);
        create_value_json(root, F("tankMiddleTemp"), nullptr, F_(tankMiddleTemp), F_(degrees), json);
        create_value_json(root, F("sysPress"), nullptr, F_(sysPress), F_(bar), json);
        create_value_json(root, F("boilTemp"), nullptr, F_(boilTemp), F_(degrees), json);
        create_value_json(root, F("burnGas"), nullptr, F_(burnGas), nullptr, json);
        create_value_json(root, F("flameCurr"), nullptr, F_(flameCurr), F_(uA), json);
        create_value_json(root, F("heatingPump"), nullptr, F_(heatingPump), nullptr, json);
        create_value_json(root, F("fanWork"), nullptr, F_(fanWork), nullptr, json);
        create_value_json(root, F("ignWork"), nullptr, F_(ignWork), nullptr, json);
        create_value_json(root, F("heatingActivated"), nullptr, F_(heatingActivated), nullptr, json);
        create_value_json(root, F("heatingTemp"), nullptr, F_(heatingTemp), F_(degrees), json);
        create_value_json(root, F("pumpModMax"), nullptr, F_(pumpModMax), F_(percent), json);
        create_value_json(root, F("pumpModMin"), nullptr, F_(pumpModMin), F_(percent), json);
        create_value_json(root, F("pumpDelay"), nullptr, F_(pumpDelay), F_(min), json);
        create_value_json(root, F("burnMinPeriod"), nullptr, F_(burnMinPeriod), F_(min), json);
        create_value_json(root, F("burnMinPower"), nullptr, F_(burnMinPower), F_(percent), json);
        create_value_json(root, F("burnMaxPower"), nullptr, F_(burnMaxPower), F_(percent), json);
        create_value_json(root, F("boilHystOn"), nullptr, F_(boilHystOn), F_(degrees), json);
        create_value_json(root, F("boilHystOff"), nullptr, F_(boilHystOff), F_(degrees), json);
        create_value_json(root, F("setFlowTemp"), nullptr, F_(setFlowTemp), F_(degrees), json);
        create_value_json(root, F("setBurnPow"), nullptr, F_(setBurnPow), F_(percent), json);
        create_value_json(root, F("burnStarts"), nullptr, F_(burnStarts), nullptr, json);
        create_value_json(root, F("burnWorkMin"), nullptr, F_(burnWorkMin), nullptr, json);
        create_value_json(root, F("heatWorkMin"), nullptr, F_(heatWorkMin), nullptr, json);
        create_value_json(root, F("UBAuptime"), nullptr, F_(UBAuptime), nullptr, json);
        // optional in info
        // create_value_json(root, F("maintenanceMessage"), nullptr, F_(maintenanceMessage), nullptr, json);
        // create_value_json(root, F("maintenance"), nullptr, F_(maintenance), (maintenanceType_ == 1) ? F_(hours) : nullptr, json);
    } else if (part == 1) {
        part = 2;
        if (!export_values_ww(json, true)) { // append ww values
            return;
        }
        // ww
        create_value_json(root, F("wWSelTemp"), nullptr, F_(wWSelTemp), F_(degrees), json);
        create_value_json(root, F("wWSetTemp"), nullptr, F_(wWSetTemp), F_(degrees), json);
        create_value_json(root, F("wWDisinfectionTemp"), nullptr, F_(wWDisinfectionTemp), F_(degrees), json);
        create_value_json(root, F("wWType"), nullptr, F_(wWType), nullptr, json);
        create_value_json(root, F("wWChargeType"), nullptr, F_(wWChargeType), nullptr, json);
        create_value_json(root, F("wWCircPump"), nullptr, F_(wWCircPump), nullptr, json);
        create_value_json(root, F("wWCircPumpMode"), nullptr, F_(wWCircPumpMode), nullptr, json);
        create_value_json(root, F("wWCirc"), nullptr, F_(wWCirc), nullptr, json);
        create_value_json(root, F("wWCurTemp"), nullptr, F_(wWCurTemp), F_(degrees), json);
        create_value_json(root, F("wWCurTemp2"), nullptr, F_(wWCurTemp2), F_(degrees), json);
        create_value_json(root, F("wWCurFlow"), nullptr, F_(wWCurFlow), F("l/min"), json);
        create_value_json(root, F("wWStorageTemp1"), nullptr, F_(wWStorageTemp1), F_(degrees), json);
        create_value_json(root, F("wWStorageTemp2"), nullptr, F_(wWStorageTemp2), F_(degrees), json);
        create_value_json(root, F("exhaustTemp"), nullptr, F_(exhaustTemp), F_(degrees), json);
        create_value_json(root, F("wWActivated"), nullptr, F_(wWActivated), nullptr, json);
        create_value_json(root, F("wWOneTime"), nullptr, F_(wWOneTime), nullptr, json);
        create_value_json(root, F("wWDisinfecting"), nullptr, F_(wWDisinfecting), nullptr, json);
        create_value_json(root, F("wWCharging"), nullptr, F_(wWCharging), nullptr, json);
        create_value_json(root, F("wWRecharging"), nullptr, F_(wWRecharging), nullptr, json);
        create_value_json(root, F("wWTempOK"), nullptr, F_(wWTempOK), nullptr, json);
        create_value_json(root, F("wWActive"), nullptr, F_(wWActive), nullptr, json);
        create_value_json(root, F("wWHeat"), nullptr, F_(wWHeat), nullptr, json);
        create_value_json(root, F("wWSetPumpPower"), nullptr, F_(wWSetPumpPower), F_(percent), json);
        create_value_json(root, F("wWStarts"), nullptr, F_(wWStarts), nullptr, json);
        create_value_json(root, F("wWWorkM"), nullptr, F_(wWWorkM), nullptr, json);
    } else if (part == 2) {
        part = 0;                              // no more parts
        if (!export_values_info(json, true)) { // append info values
            return;
        }
        create_value_json(root, F("upTimeControl"), nullptr, F_(upTimeControl), nullptr, json);
        create_value_json(root, F("upTimeCompHeating"), nullptr, F_(upTimeCompHeating), nullptr, json);
        create_value_json(root, F("upTimeCompCooling"), nullptr, F_(upTimeCompCooling), nullptr, json);
        create_value_json(root, F("upTimeCompWw"), nullptr, F_(upTimeCompWw), nullptr, json);
        create_value_json(root, F("heatingStarts"), nullptr, F_(heatingStarts), nullptr, json);
        create_value_json(root, F("coolingStarts"), nullptr, F_(coolingStarts), nullptr, json);
        create_value_json(root, F("wWStarts2"), nullptr, F_(wWStarts2), nullptr, json);
        create_value_json(root, F("nrgConsTotal"), nullptr, F_(nrgConsTotal), F_(kwh), json);
        create_value_json(root, F("auxElecHeatNrgConsTotal"), nullptr, F_(auxElecHeatNrgConsTotal), F_(kwh), json);
        create_value_json(root, F("auxElecHeatNrgConsHeating"), nullptr, F_(auxElecHeatNrgConsHeating), F_(kwh), json);
        create_value_json(root, F("auxElecHeatNrgConsDHW"), nullptr, F_(auxElecHeatNrgConsDHW), F_(kwh), json);
        create_value_json(root, F("nrgConsCompTotal"), nullptr, F_(nrgConsCompTotal), F_(kwh), json);
        create_value_json(root, F("nrgConsCompHeating"), nullptr, F_(nrgConsCompHeating), F_(kwh), json);
        create_value_json(root, F("nrgConsCompWw"), nullptr, F_(nrgConsCompWw), F_(kwh), json);
        create_value_json(root, F("nrgConsCoolingTotal"), nullptr, F_(nrgConsCompCooling), F_(kwh), json);
        create_value_json(root, F("nrgSuppTotal"), nullptr, F_(nrgSuppTotal), F_(kwh), json);
        create_value_json(root, F("nrgSuppHeating"), nullptr, F_(nrgSuppHeating), F_(kwh), json);
        create_value_json(root, F("nrgSuppWw"), nullptr, F_(nrgSuppWw), F_(kwh), json);
        create_value_json(root, F("nrgSuppCooling"), nullptr, F_(nrgSuppCooling), F_(kwh), json);
        create_value_json(root, F("maintenanceMessage"), nullptr, F_(maintenanceMessage), nullptr, json);
        create_value_json(root, F("maintenance"), nullptr, F_(maintenance), nullptr, json);
        create_value_json(root, F("maintenanceTime"), nullptr, F_(maintenanceTime), F_(hours), json);
        create_value_json(root, F("maintenanceDate"), nullptr, F_(maintenanceDate), nullptr, json);
    }
}

bool Boiler::export_values(JsonObject & json, int8_t id) {
    if (!export_values_main(json)) {
        return false;
    }
    export_values_ww(json);   // append ww values
    export_values_info(json); // append info values
    return true;
}

// creates JSON doc from values
// returns false if empty
bool Boiler::export_values_ww(JsonObject & json, const bool textformat) {
    char s[10]; // for formatting strings

    // Warm Water comfort setting
    if (Helpers::hasValue(wWComfort_)) {
        if (wWComfort_ == 0x00) {
            json["wWComfort"] = FJSON("Hot");
        } else if (wWComfort_ == 0xD8) {
            json["wWComfort"] = FJSON("Eco");
        } else if (wWComfort_ == 0xEC) {
            json["wWComfort"] = FJSON("Intelligent");
        }
    }

    // Warm Water selected temperature
    if (Helpers::hasValue(wWSelTemp_)) {
        json["wWSelTemp"] = wWSelTemp_;
    }

    // Warm Water set temperature
    if (Helpers::hasValue(wWSetTemp_)) {
        json["wWSetTemp"] = wWSetTemp_;
    }

    // Warm Water disinfection temperature
    if (Helpers::hasValue(wWDisinfectionTemp_)) {
        json["wWDisinfectionTemp"] = wWDisinfectionTemp_;
    }

    // Warm Water type
    Helpers::json_enum(json, "wWType", {F("off"), F("flow"), F("buffered flow"), F("buffer"), F("layered buffer")}, wWType_);

    // Warm Water charging type
    if (Helpers::hasValue(wWChargeType_, EMS_VALUE_BOOL)) {
        json["wWChargeType"] = wWChargeType_ ? FJSON("3-way valve") : FJSON("charge pump");
    }

    // Warm Water circulation pump available bool
    Helpers::json_boolean(json, "wWCircPump", wWCircPump_);

    // Warm Water circulation pump freq
    if (Helpers::hasValue(wWCircPumpMode_)) {
        if (wWCircPumpMode_ == 7) {
            json["wWCircPumpMode"] = FJSON("continuous");
        } else {
            snprintf_P(s, sizeof(s), PSTR("%dx3min"), wWCircPumpMode_);
            json["wWCircPumpMode"] = s;
        }
    }

    // Warm Water circulation active bool
    Helpers::json_boolean(json, "wWCirc", wWCirc_);

    // Warm Water current temperature (intern)
    if (Helpers::hasValue(wWCurTemp_)) {
        json["wWCurTemp"] = (float)wWCurTemp_ / 10;
    }

    // Warm Water current temperature (extern)
    if (Helpers::hasValue(wWCurTemp2_)) {
        json["wWCurTemp2"] = (float)wWCurTemp2_ / 10;
    }

    // Warm Water current tap water flow l/min
    if (Helpers::hasValue(wWCurFlow_)) {
        json["wWCurFlow"] = (float)wWCurFlow_ / 10;
    }

    // Warm water storage temperature (intern)
    if (Helpers::hasValue(wWStorageTemp1_)) {
        json["wWStorageTemp1"] = (float)wWStorageTemp1_ / 10;
    }

    // Warm water storage temperature (extern)
    if (Helpers::hasValue(wWStorageTemp2_)) {
        json["wWStorageTemp2"] = (float)wWStorageTemp2_ / 10;
    }

    // Warm Water activated bool
    Helpers::json_boolean(json, "wWActivated", wWActivated_);

    // Warm Water one time charging bool
    Helpers::json_boolean(json, "wWOneTime", wWOneTime_);

    // Warm Water disinfecting bool
    Helpers::json_boolean(json, "wWDisinfecting", wWDisinfecting_);

    // Warm water charging bool
    Helpers::json_boolean(json, "wWCharging", wWCharging_);

    // Warm water recharge bool
    Helpers::json_boolean(json, "wWRecharging", wWRecharging_);

    // Warm water temperature ok bool
    Helpers::json_boolean(json, "wWTempOK", wWTempOK_);

    // Warm water active bool
    Helpers::json_boolean(json, "wWActive", wWActive_);

    // Warm Water charging bool
    Helpers::json_boolean(json, "wWHeat", wWHeat_);

    // Warm Water pump set power %
    if (Helpers::hasValue(wWSetPumpPower_)) {
        json["wWSetPumpPower"] = wWSetPumpPower_;
    }

    // Warm Water # starts
    if (Helpers::hasValue(wWStarts_)) {
        json["wWStarts"] = wWStarts_;
    }

    // Warm Water active time
    Helpers::json_time(json, "wWWorkM", wWWorkM_, textformat);

    return (json.size());
}

// creates JSON doc from values
// returns false if empty
bool Boiler::export_values_main(JsonObject & json, const bool textformat) {
    // Hot tap water bool
    Helpers::json_boolean(json, "heatingActive", heatingActive_);

    // Central heating bool
    Helpers::json_boolean(json, "tapwaterActive", tapwaterActive_);

    // Selected flow temperature deg
    if (Helpers::hasValue(selFlowTemp_)) {
        json["selFlowTemp"] = selFlowTemp_;
    }

    // Burner selected max power %
    if (Helpers::hasValue(selBurnPow_)) {
        json["selBurnPow"] = selBurnPow_;
    }

    // Burner current power %
    if (Helpers::hasValue(curBurnPow_)) {
        json["curBurnPow"] = curBurnPow_;
    }

    // Heating pump modulation %
    if (Helpers::hasValue(heatingPumpMod_)) {
        json["heatingPumpMod"] = heatingPumpMod_;
    }

    // Heating Pump 2 modulation %
    if (Helpers::hasValue(heatingPump2Mod_)) {
        json["heatingPump2Mod"] = heatingPump2Mod_;
    }

    // Outside temperature
    if (Helpers::hasValue(outdoorTemp_)) {
        json["outdoorTemp"] = (float)outdoorTemp_ / 10;
    }

    // Current flow temperature
    if (Helpers::hasValue(curFlowTemp_)) {
        json["curFlowTemp"] = (float)curFlowTemp_ / 10;
    }

    // Return temperature, with no sensor retTemp can be 0x8000 or 0x0000
    if (Helpers::hasValue(retTemp_) && (retTemp_ > 0)) {
        json["retTemp"] = (float)retTemp_ / 10;
    }

    // Mixing switch temperature
    if (Helpers::hasValue(switchTemp_)) {
        json["switchTemp"] = (float)switchTemp_ / 10;
    }

    // Mixer temperature
    if (Helpers::hasValue(mixerTemp_)) {
        json["mixerTemp"] = (float)mixerTemp_ / 10;
    }

    // tank middle temperature (TS3)
    if (Helpers::hasValue(tankMiddleTemp_)) {
        json["tankMiddleTemp"] = (float)tankMiddleTemp_ / 10;
    }

    // System pressure
    if (Helpers::hasValue(sysPress_)) {
        json["sysPress"] = (float)sysPress_ / 10;
    }

    // Max boiler temperature
    if (Helpers::hasValue(boilTemp_)) {
        json["boilTemp"] = (float)boilTemp_ / 10;
    }

    // Exhaust temperature
    if (Helpers::hasValue(exhaustTemp_)) {
        json["exhaustTemp"] = (float)exhaustTemp_ / 10;
    }

    // Gas bool
    Helpers::json_boolean(json, "burnGas", burnGas_);

    // Flame current uA
    if (Helpers::hasValue(flameCurr_)) {
        json["flameCurr"] = (float)(int16_t)flameCurr_ / 10;
    }

    // Heating pump bool
    Helpers::json_boolean(json, "heatingPump", heatingPump_);

    // Fan bool
    Helpers::json_boolean(json, "fanWork", fanWork_);

    // Ignition bool
    Helpers::json_boolean(json, "ignWork", ignWork_);

    // heating activated bool
    Helpers::json_boolean(json, "heatingActivated", heatingActivated_);

    // Heating temperature setting on the boiler
    if (Helpers::hasValue(heatingTemp_)) {
        json["heatingTemp"] = heatingTemp_;
    }

    // Boiler circuit pump modulation max power %
    if (Helpers::hasValue(pumpModMax_)) {
        json["pumpModMax"] = pumpModMax_;
    }

    // Boiler circuit pump modulation min power %
    if (Helpers::hasValue(pumpModMin_)) {
        json["pumpModMin"] = pumpModMin_;
    }

    // Boiler circuit pump delay time min
    if (Helpers::hasValue(pumpDelay_)) {
        json["pumpDelay"] = pumpDelay_;
    }

    // Boiler burner min period min
    if (Helpers::hasValue(burnMinPeriod_)) {
        json["burnMinPeriod"] = burnMinPeriod_;
    }

    // Boiler burner min power %
    if (Helpers::hasValue(burnMinPower_)) {
        json["burnMinPower"] = burnMinPower_;
    }

    // Boiler burner max power %
    if (Helpers::hasValue(burnMaxPower_)) {
        json["burnMaxPower"] = burnMaxPower_;
    }

    // Boiler temp hysteresis on degrees
    if (Helpers::hasValue(boilHystOn_)) {
        json["boilHystOn"] = boilHystOn_;
    }

    // Boiler temp hysteresis off degrees
    if (Helpers::hasValue(boilHystOff_)) {
        json["boilHystOff"] = boilHystOff_;
    }

    // Set Flow temperature
    if (Helpers::hasValue(setFlowTemp_)) {
        json["setFlowTemp"] = setFlowTemp_;
    }

    // burn power %
    if (Helpers::hasValue(setBurnPow_)) {
        json["setBurnPow"] = setBurnPow_;
    }

    // Burner # starts
    if (Helpers::hasValue(burnStarts_)) {
        json["burnStarts"] = burnStarts_;
    }

    // Total burner operating time
    Helpers::json_time(json, "burnWorkMin", burnWorkMin_, textformat);

    // Total heat operating time
    Helpers::json_time(json, "heatWorkMin", heatWorkMin_, textformat);

    // Total UBA working time
    Helpers::json_time(json, "UBAuptime", UBAuptime_, textformat);

    /*
    // Service Code & Service Code Number. Priority error - maintenance - workingcode
    if ((serviceCode_[0] >= '1' && serviceCode_[0] <= '9') || (serviceCode_[0] >= 'A' && serviceCode_[0] <= 'Z')) {
        json["serviceCode"] = serviceCode_;
    } else if (Helpers::hasValue(maintenanceMessage_) && maintenanceMessage_ > 0) {
        char s[5];
        snprintf_P(s, sizeof(s), PSTR("H%02d"), maintenanceMessage_);
        json["serviceCode"] = s;
    } else if (serviceCode_[0] == 0xF0) {
        json["serviceCode"] = FJSON("~H");
    } else {
        json["serviceCode"] = serviceCode_;
    }
	*/

    if (Helpers::hasValue(serviceCodeNumber_)) {
        if (serviceCode_[0] == 0xF0) {
            json["serviceCode"] = FJSON("~H");
        } else {
            json["serviceCode"] = serviceCode_;
        }
        json["serviceCodeNumber"] = serviceCodeNumber_;
    }

    if (lastCode_[0] != '\0') {
        json["lastCode"] = lastCode_;
    }
    /*
    if (Helpers::hasValue(maintenanceMessage_)) {
        if (maintenanceMessage_ > 0) {
            char s[5];
            snprintf_P(s, sizeof(s), PSTR("H%02d"), maintenanceMessage_);
            json["maintenanceMessage"] = s;
        } else {
            json["maintenanceMessage"] = " ";
        }
    }

    if (Helpers::hasValue(maintenanceType_)) {
        if (maintenanceType_ == 0) {
            json["maintenance"] = FJSON("off");
        } else if (maintenanceType_ == 1) {
            json["maintenance"] = maintenanceTime_ * 100;
        } else if (maintenanceType_ == 2) {
            json["maintenance"] = maintenanceDate_;
        }
    }
	*/
    return (json.size());
}

// creates JSON doc from values,  returns false if empty
bool Boiler::export_values_info(JsonObject & json, const bool textformat) {
    // Total heat operating time
    Helpers::json_time(json, "upTimeControl", upTimeControl_ / 60, textformat);

    // Operating time compressor heating
    Helpers::json_time(json, "upTimeCompHeating", upTimeCompHeating_ / 60, textformat);

    // Operating time compressor cooling
    Helpers::json_time(json, "pTimeCompCooling", upTimeCompCooling_ / 60, textformat);

    // Operating time compressor warm water
    Helpers::json_time(json, "upTimeCompWw", upTimeCompWw_ / 60, textformat);

    // Number of heating starts
    if (Helpers::hasValue(heatingStarts_)) {
        json["heatingStarts"] = heatingStarts_;
    }

    // Number of cooling starts
    if (Helpers::hasValue(coolingStarts_)) {
        json["coolingStarts"] = coolingStarts_;
    }

    // Number of warm water starts
    if (Helpers::hasValue(wWStarts2_)) {
        json["wWStarts2"] = wWStarts2_;
    }

    // Total energy consumption
    if (Helpers::hasValue(nrgConsTotal_)) {
        json["nrgConsTotal"] = nrgConsTotal_;
    }

    // Auxiliary electrical heater energy total
    if (Helpers::hasValue(auxElecHeatNrgConsTotal_)) {
        json["auxElecHeatNrgConsTotal"] = auxElecHeatNrgConsTotal_;
    }

    // Auxiliary electrical heater energy heating
    if (Helpers::hasValue(auxElecHeatNrgConsHeating_)) {
        json["auxElecHeatNrgConsHeating"] = auxElecHeatNrgConsHeating_;
    }

    // Auxiliary electrical heater energy DHW
    if (Helpers::hasValue(auxElecHeatNrgConsDHW_)) {
        json["auxElecHeatNrgConsDHW"] = auxElecHeatNrgConsDHW_;
    }

    // Energy consumption compressor total
    if (Helpers::hasValue(nrgConsCompTotal_)) {
        json["nrgConsCompTotal"] = nrgConsCompTotal_;
    }

    // Energy consumption compressor heating
    if (Helpers::hasValue(nrgConsCompHeating_)) {
        json["nrgConsCompHeating"] = nrgConsCompHeating_;
    }

    // Energy consumption compressor warm water
    if (Helpers::hasValue(nrgConsCompWw_)) {
        json["nrgConsCompWw"] = nrgConsCompWw_;
    }

    // Energy consumption compressor cooling
    if (Helpers::hasValue(nrgConsCompCooling_)) {
        json["nrgConsCompCooling"] = nrgConsCompCooling_;
    }

    // Total energy supplied
    if (Helpers::hasValue(nrgSuppTotal_)) {
        json["nrgSuppTotal"] = nrgSuppTotal_;
    }

    // Total energy heating
    if (Helpers::hasValue(nrgSuppHeating_)) {
        json["nrgSuppHeating"] = nrgSuppHeating_;
    }

    // Total energy warm water
    if (Helpers::hasValue(nrgSuppWw_)) {
        json["nrgSuppWw"] = nrgSuppWw_;
    }

    // Total energy cooling
    if (Helpers::hasValue(nrgSuppCooling_)) {
        json["nrgSuppCooling"] = nrgSuppCooling_;
    }

    // show always all maintenance values like v3
    if (Helpers::hasValue(maintenanceMessage_)) {
        char s[5];
        snprintf_P(s, sizeof(s), PSTR("H%02d"), maintenanceMessage_);
        json["maintenanceMessage"] = maintenanceMessage_ ? s : "-";
    }

    Helpers::json_enum(json, "maintenance", {F("off"), F("time"), F("date")}, maintenanceType_);

    if (Helpers::hasValue(maintenanceTime_)) {
        json["maintenanceTime"] = maintenanceTime_ * 100;
    }

    if (maintenanceDate_[0] != '\0') {
        json["maintenanceDate"] = maintenanceDate_;
    }

    return (json.size());
}

// publish values via MQTT
void Boiler::publish_values(JsonObject & json, bool force) {
    // handle HA first
    if (Mqtt::mqtt_format() == Mqtt::Format::HA) {
        if (force) {
            mqtt_ha_config_    = false;
            mqtt_ha_config_ww_ = false;
        }
        // register ww in next cycle if both unregistered
        if (!mqtt_ha_config_) {
            register_mqtt_ha_config();
            return;
        } else if (!mqtt_ha_config_ww_) {
            register_mqtt_ha_config_ww();
            return;
        }
    }

    StaticJsonDocument<EMSESP_MAX_JSON_SIZE_LARGE> doc;
    JsonObject                                     json_data = doc.to<JsonObject>();
    if (export_values_main(json_data)) {
        Mqtt::publish(F("boiler_data"), json_data);
    }
    json_data.clear();

    if (export_values_ww(json_data)) {
        Mqtt::publish(F("boiler_data_ww"), json_data);
    }
    json_data.clear();

    if (export_values_info(json_data)) {
        Mqtt::publish(F("boiler_data_info"), json_data);
    }
    // send out heating and tapwater status
    check_active(force);
}

// called after a process command is called, to check values and see if we need to force an MQTT publish
bool Boiler::updated_values() {
    if (changed_) {
        changed_ = false;
        return true;
    }
    return false;
}

/*
 * Check if hot tap water or heating is active
 * If a value has changed, post it immediately to MQTT so we get real time data
 * Values will always be posted first time as heatingActive_ and tapwaterActive_ will have values EMS_VALUE_BOOL_NOTSET
 */
void Boiler::check_active(const bool force) {
    if (!Helpers::hasValue(boilerState_)) {
        return;
    }
    bool    b;
    uint8_t val;

    // check if heating is active, bits 2 and 4 must be set
    b   = ((boilerState_ & 0x09) == 0x09);
    val = b ? EMS_VALUE_BOOL_ON : EMS_VALUE_BOOL_OFF;
    if (heatingActive_ != val || force) {
        heatingActive_ = val;
        char s[7];
        Mqtt::publish(F("heating_active"), Helpers::render_boolean(s, b));
    }

    // check if tap water is active, bits 1 and 4 must be set
    // also check if there is a flowsensor and flow-type
    static bool flowsensor = false;
    if (Helpers::hasValue(wWCurFlow_) && (wWCurFlow_ > 0) && (wWType_ == 1)) {
        flowsensor = true;
    }
    if (flowsensor) {
        b = ((wWCurFlow_ > 0) && ((boilerState_ & 0x0A) == 0x0A));
    } else {
        b = ((boilerState_ & 0x0A) == 0x0A);
    }
    val = b ? EMS_VALUE_BOOL_ON : EMS_VALUE_BOOL_OFF;
    if (tapwaterActive_ != val || force) {
        tapwaterActive_ = val;
        char s[7];
        Mqtt::publish(F("tapwater_active"), Helpers::render_boolean(s, b));
        EMSESP::tap_water_active(b); // let EMS-ESP know, used in the Shower class
    }
}

// 0x33
void Boiler::process_UBAParameterWW(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(wWActivated_, 1);    // 0xFF means on
    changed_ |= telegram->read_value(wWCircPump_, 6);     // 0xFF means on
    changed_ |= telegram->read_value(wWCircPumpMode_, 7); // 1=1x3min... 6=6x3min, 7=continuous
    changed_ |= telegram->read_value(wWChargeType_, 10);  // 0 = charge pump, 0xff = 3-way valve
    changed_ |= telegram->read_value(wWSelTemp_, 2);
    changed_ |= telegram->read_value(wWDisinfectionTemp_, 8);
    changed_ |= telegram->read_value(wWComfort_, 9);
}

// 0x18
void Boiler::process_UBAMonitorFast(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(selFlowTemp_, 0);
    changed_ |= telegram->read_value(curFlowTemp_, 1);
    changed_ |= telegram->read_value(selBurnPow_, 3); // burn power max setting
    changed_ |= telegram->read_value(curBurnPow_, 4);
    changed_ |= telegram->read_value(boilerState_, 5);

    changed_ |= telegram->read_bitvalue(burnGas_, 7, 0);
    changed_ |= telegram->read_bitvalue(fanWork_, 7, 2);
    changed_ |= telegram->read_bitvalue(ignWork_, 7, 3);
    changed_ |= telegram->read_bitvalue(heatingPump_, 7, 5);
    changed_ |= telegram->read_bitvalue(wWHeat_, 7, 6);
    changed_ |= telegram->read_bitvalue(wWCirc_, 7, 7);

    // warm water storage sensors (if present)
    // wWStorageTemp2 is also used by some brands as the boiler temperature - see https://github.com/emsesp/EMS-ESP/issues/206
    changed_ |= telegram->read_value(wWStorageTemp1_, 9);  // 0x8300 if not available
    changed_ |= telegram->read_value(wWStorageTemp2_, 11); // 0x8000 if not available - this is boiler temp

    changed_ |= telegram->read_value(retTemp_, 13);
    changed_ |= telegram->read_value(flameCurr_, 15);

    // system pressure. FF means missing
    changed_ |= telegram->read_value(sysPress_, 17); // is *10

    // read the service code / installation status as appears on the display
    if ((telegram->message_length > 18) && (telegram->offset == 0)) {
        changed_ |= telegram->read_value(serviceCode_[0], 18);
        changed_ |= telegram->read_value(serviceCode_[1], 19);
        serviceCode_[2] = '\0'; // null terminate string
    }

    changed_ |= telegram->read_value(serviceCodeNumber_, 20);

    // at this point do a quick check to see if the hot water or heating is active
    check_active();
}

/*
 * UBATotalUptime - type 0x14 - total uptime
 * received only after requested (not broadcasted)
 */
void Boiler::process_UBATotalUptime(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(UBAuptime_, 0, 3); // force to 3 bytes
}

/*
 * UBAParameters - type 0x16
 */
void Boiler::process_UBAParameters(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(heatingActivated_, 0);
    changed_ |= telegram->read_value(heatingTemp_, 1);
    changed_ |= telegram->read_value(burnMaxPower_, 2);
    changed_ |= telegram->read_value(burnMinPower_, 3);
    changed_ |= telegram->read_value(boilHystOff_, 4);
    changed_ |= telegram->read_value(boilHystOn_, 5);
    changed_ |= telegram->read_value(burnMinPeriod_, 6);
    changed_ |= telegram->read_value(pumpDelay_, 8);
    changed_ |= telegram->read_value(pumpModMax_, 9);
    changed_ |= telegram->read_value(pumpModMin_, 10);
}

/*
 * UBAMonitorWW - type 0x34 - warm water monitor. 19 bytes long
 * received every 10 seconds
 */
void Boiler::process_UBAMonitorWW(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(wWSetTemp_, 0);
    changed_ |= telegram->read_value(wWCurTemp_, 1);
    changed_ |= telegram->read_value(wWCurTemp2_, 3);
    changed_ |= telegram->read_value(wWCurFlow_, 9);
    changed_ |= telegram->read_value(wWType_, 8);

    changed_ |= telegram->read_value(wWWorkM_, 10, 3);  // force to 3 bytes
    changed_ |= telegram->read_value(wWStarts_, 13, 3); // force to 3 bytes

    changed_ |= telegram->read_bitvalue(wWOneTime_, 5, 1);
    changed_ |= telegram->read_bitvalue(wWDisinfecting_, 5, 2);
    changed_ |= telegram->read_bitvalue(wWCharging_, 5, 3);
    changed_ |= telegram->read_bitvalue(wWRecharging_, 5, 4);
    changed_ |= telegram->read_bitvalue(wWTempOK_, 5, 5);
    changed_ |= telegram->read_bitvalue(wWActive_, 5, 6);
}

/*
 * UBAMonitorFastPlus - type 0xE4 - central heating monitor EMS+
 * temperatures at 7 and 23 always identical
 * Bosch Logamax Plus GB122: issue #620
 * 88 00 E4 00 00 2D 2D 00 00 C9 34 02 21 64 3D 05 02 01 DE 00 00 00 00 03 62 14 00 02 21 00 00 00 00 00 00 00 2B 2B 83
 * GB125/Logamatic MC110: issue #650: add retTemp & sysPress
 * 08 00 E4 00 10 20 2D 48 00 C8 38 02 37 3C 27 03 00 00 00 00 00 01 7B 01 8F 11 00 02 37 80 00 02 1B 80 00 7F FF 80 00
 */
void Boiler::process_UBAMonitorFastPlus(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(selFlowTemp_, 6);
    changed_ |= telegram->read_bitvalue(burnGas_, 11, 0);
    // changed_ |= telegram->read_bitvalue(heatingPump_, 11, 1); // heating active? see SlowPlus
    changed_ |= telegram->read_bitvalue(wWHeat_, 11, 2);
    changed_ |= telegram->read_value(curBurnPow_, 10);
    changed_ |= telegram->read_value(selBurnPow_, 9);
    changed_ |= telegram->read_value(curFlowTemp_, 7);
    changed_ |= telegram->read_value(flameCurr_, 19);
    changed_ |= telegram->read_value(retTemp_, 17); // can be 0 if no sensor, handled in export_values
    changed_ |= telegram->read_value(sysPress_, 21);

    //changed_ |= telegram->read_value(temperature_, 13); // unknown temperature
    //changed_ |= telegram->read_value(temperature_, 27); // unknown temperature

    // read 3 char service code / installation status as appears on the display
    if ((telegram->message_length > 3) && (telegram->offset == 0)) {
        changed_ |= telegram->read_value(serviceCode_[0], 1);
        changed_ |= telegram->read_value(serviceCode_[1], 2);
        changed_ |= telegram->read_value(serviceCode_[2], 3);
        serviceCode_[3] = '\0';
    }
    changed_ |= telegram->read_value(serviceCodeNumber_, 4);

    // at this point do a quick check to see if the hot water or heating is active
    uint8_t state = EMS_VALUE_UINT_NOTSET;
    if (telegram->read_value(state, 11)) {
        boilerState_ = state & 0x01 ? 0x08 : 0;
        boilerState_ |= state & 0x02 ? 0x01 : 0;
        boilerState_ |= state & 0x04 ? 0x02 : 0;
    }

    check_active();
}

/*
 * UBAMonitorSlow - type 0x19 - central heating monitor part 2 (27 bytes long)
 * received every 60 seconds
 * e.g. 08 00 19 00 80 00 02 41 80 00 00 00 00 00 03 91 7B 05 B8 40 00 00 00 04 92 AD 00 5E EE 80 00
 *      08 0B 19 00 FF EA 02 47 80 00 00 00 00 62 03 CA 24 2C D6 23 00 00 00 27 4A B6 03 6E 43
 *                  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 17 19 20 21 22 23 24
 */
void Boiler::process_UBAMonitorSlow(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(outdoorTemp_, 0);
    changed_ |= telegram->read_value(boilTemp_, 2);
    changed_ |= telegram->read_value(exhaustTemp_, 4);
    changed_ |= telegram->read_value(switchTemp_, 25); // only if there is a mixer module present
    changed_ |= telegram->read_value(heatingPumpMod_, 9);
    changed_ |= telegram->read_value(burnStarts_, 10, 3);  // force to 3 bytes
    changed_ |= telegram->read_value(burnWorkMin_, 13, 3); // force to 3 bytes
    changed_ |= telegram->read_value(heatWorkMin_, 19, 3); // force to 3 bytes
}

/*
 * UBAMonitorSlowPlus2 - type 0xE3
 * 88 00 E3 00 04 00 00 00 00 01 00 00 00 00 00 02 22 2B 64 46 01 00 00 61
 */
void Boiler::process_UBAMonitorSlowPlus2(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(heatingPump2Mod_, 13); // Heating Pump 2 Modulation
}

/*
 * UBAMonitorSlowPlus - type 0xE5 - central heating monitor EMS+
 * Boiler(0x08) -> Me(0x0B), UBAMonitorSlowPlus(0xE5),
 * data: 01 00 20 00 00 78 00 00 00 00 00 1E EB 00 9D 3E 00 00 00 00 6B 5E 00 06 4C 64 00 00 00 00 8A A3
 */
void Boiler::process_UBAMonitorSlowPlus(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_bitvalue(fanWork_, 2, 2);
    changed_ |= telegram->read_bitvalue(ignWork_, 2, 3);
    changed_ |= telegram->read_bitvalue(heatingPump_, 2, 5);
    changed_ |= telegram->read_bitvalue(wWCirc_, 2, 7);
    changed_ |= telegram->read_value(exhaustTemp_, 6);
    changed_ |= telegram->read_value(burnStarts_, 10, 3);  // force to 3 bytes
    changed_ |= telegram->read_value(burnWorkMin_, 13, 3); // force to 3 bytes
    changed_ |= telegram->read_value(heatWorkMin_, 19, 3); // force to 3 bytes
    changed_ |= telegram->read_value(heatingPumpMod_, 25);
    // temperature measurements at 4, see #620, outdoortemp?
}

/*
 * UBAParametersPlus - type 0xe6
 * 88 0B E6 00 01 46 00 00 46 0A 00 01 06 FA 0A 01 02 64 01 00 00 1E 00 3C 01 00 00 00 01 00 9A
 */
void Boiler::process_UBAParametersPlus(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(heatingActivated_, 0);
    changed_ |= telegram->read_value(heatingTemp_, 1);
    changed_ |= telegram->read_value(burnMaxPower_, 4);
    changed_ |= telegram->read_value(burnMinPower_, 5);
    changed_ |= telegram->read_value(boilHystOff_, 8);
    changed_ |= telegram->read_value(boilHystOn_, 9);
    changed_ |= telegram->read_value(burnMinPeriod_, 10);
    // changed_ |= telegram->read_value(pumpModMax_, 13); // guess
    // changed_ |= telegram->read_value(pumpModMin_, 14); // guess
}

// 0xEA
void Boiler::process_UBAParameterWWPlus(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(wWActivated_, 5);     // 0x01 means on
    changed_ |= telegram->read_value(wWCircPump_, 10);     // 0x01 means yes
    changed_ |= telegram->read_value(wWCircPumpMode_, 11); // 1=1x3min... 6=6x3min, 7=continuous
    // changed_ |= telegram->read_value(wWDisinfectTemp_, 12); // settings, status in E9
    // changed_ |= telegram->read_value(wWSelTemp_, 6);        // settings, status in E9
}

// 0xE9 - DHW Status
// e.g. 08 00 E9 00 37 01 F6 01 ED 00 00 00 00 41 3C 00 00 00 00 00 00 00 00 00 00 00 00 37 00 00 00 (CRC=77) #data=27
void Boiler::process_UBADHWStatus(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(wWSetTemp_, 0);
    changed_ |= telegram->read_value(wWCurTemp_, 1);
    changed_ |= telegram->read_value(wWCurTemp2_, 3);

    changed_ |= telegram->read_value(wWWorkM_, 14, 3);  // force to 3 bytes
    changed_ |= telegram->read_value(wWStarts_, 17, 3); // force to 3 bytes

    changed_ |= telegram->read_bitvalue(wWOneTime_, 12, 2);
    changed_ |= telegram->read_bitvalue(wWDisinfecting_, 12, 3);
    changed_ |= telegram->read_bitvalue(wWCharging_, 12, 4);
    changed_ |= telegram->read_bitvalue(wWRecharging_, 13, 4);
    changed_ |= telegram->read_bitvalue(wWTempOK_, 13, 5);
    changed_ |= telegram->read_bitvalue(wWCirc_, 13, 2);

    // changed_ |= telegram->read_value(wWActivated_, 20); // Activated is in 0xEA, this is something other 0/100%
    changed_ |= telegram->read_value(wWSelTemp_, 10);
    changed_ |= telegram->read_value(wWDisinfectionTemp_, 9);
}

/*
 * UBAInformation - type 0x495
 * all values 32 bit
 * 08 0B FF 00 03 95 01 01 AB 83 00 27 78 EB 00 84 FA 39 FF FF FF 00 00 53 7D 8D 00 00 0F 04 1C
 * 08 00 FF 00 03 95 01 01 AB 83 00 27 78 EB 00 84 FA 39 FF FF FF 00 00 53 7D 8D 00 00 0F 04 63
 * 08 00 FF 18 03 95 00 00 05 84 00 00 07 22 FF FF FF FF 00 00 02 5C 00 00 03 C0 00 00 01 98 64
 * 08 00 FF 30 03 95 00 00 00 D4 FF FF FF FF 00 00 1C 70 FF FF FF FF 00 00 20 30 00 00 0E 06 FB
 * 08 00 FF 48 03 95 00 00 06 C0 00 00 07 66 FF FF FF FF 2E
 */
void Boiler::process_UBAInformation(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(upTimeControl_, 0);
    changed_ |= telegram->read_value(upTimeCompHeating_, 8);
    changed_ |= telegram->read_value(upTimeCompCooling_, 16);
    changed_ |= telegram->read_value(upTimeCompWw_, 4);

    changed_ |= telegram->read_value(heatingStarts_, 28);
    changed_ |= telegram->read_value(coolingStarts_, 36);
    changed_ |= telegram->read_value(wWStarts2_, 24);

    changed_ |= telegram->read_value(nrgConsTotal_, 64);

    changed_ |= telegram->read_value(auxElecHeatNrgConsTotal_, 40);
    changed_ |= telegram->read_value(auxElecHeatNrgConsHeating_, 48);
    changed_ |= telegram->read_value(auxElecHeatNrgConsDHW_, 44);

    changed_ |= telegram->read_value(nrgConsCompTotal_, 56);
    changed_ |= telegram->read_value(nrgConsCompHeating_, 68);
    changed_ |= telegram->read_value(nrgConsCompWw_, 72);
    changed_ |= telegram->read_value(nrgConsCompCooling_, 76);
}

/*
 * UBAEnergy - type 0x494
 * Energy-values all 32bit
 * 08 00 FF 00 03 94 03 31 21 59 00 00 7C 70 00 00 15 B8 00 00 40 E3 00 00 27 23 FF FF FF FF EA
 * 08 00 FF 18 03 94 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 7E
 * 08 00 FF 31 03 94 00 00 00 00 00 00 00 38
 */
void Boiler::process_UBAEnergySupplied(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(nrgSuppTotal_, 4);
    changed_ |= telegram->read_value(nrgSuppHeating_, 12);
    changed_ |= telegram->read_value(nrgSuppWw_, 8);
    changed_ |= telegram->read_value(nrgSuppCooling_, 16);
}

// 0x2A - MC10Status
// e.g. 88 00 2A 00 00 00 00 00 00 00 00 00 D2 00 00 80 00 00 01 08 80 00 02 47 00
// see https://github.com/emsesp/EMS-ESP/issues/397
void Boiler::process_MC10Status(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(mixerTemp_, 14);
    changed_ |= telegram->read_value(tankMiddleTemp_, 18);
}

/*
 * UBAOutdoorTemp - type 0xD1 - external temperature EMS+
 */
void Boiler::process_UBAOutdoorTemp(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(outdoorTemp_, 0);
}

// UBASetPoint 0x1A
void Boiler::process_UBASetPoints(std::shared_ptr<const Telegram> telegram) {
    changed_ |= telegram->read_value(setFlowTemp_, 0);    // boiler set temp from thermostat
    changed_ |= telegram->read_value(setBurnPow_, 1);     // max json power in %
    changed_ |= telegram->read_value(wWSetPumpPower_, 2); // ww pump speed/power?
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// 0x35
// not yet implemented
void Boiler::process_UBAFlags(std::shared_ptr<const Telegram> telegram) {
}

#pragma GCC diagnostic pop

// 0x1C
// 08 00 1C 00 94 0B 0A 1D 31 08 00 80 00 00 00 -> message for 29.11.2020
// 08 00 1C 00 94 0B 0A 1D 31 00 00 00 00 00 00 -> message reset
void Boiler::process_UBAMaintenanceStatus(std::shared_ptr<const Telegram> telegram) {
    // 5. byte: Maintenance due (0 = no, 3 = yes, due to operating hours, 8 = yes, due to date)
    changed_ |= telegram->read_value(maintenanceMessage_, 5);
    // first bytes: date of message: 94 0B 0A 1D 31 -> 29.11.2020 10:49 (year-month-hour-day-minute)
}

// 0x10, 0x11
void Boiler::process_UBAErrorMessage(std::shared_ptr<const Telegram> telegram) {
    if (telegram->offset > 0 || telegram->message_length < 9) {
        return;
    }
    // data: displaycode(2), errornumber(2), year, month, hour, day, minute, duration(2), src-addr
    if (telegram->message_data[4] & 0x80) { // valid date
        char     code[3];
        uint16_t codeNo;
        code[0] = telegram->message_data[0];
        code[1] = telegram->message_data[1];
        code[2] = 0;
        telegram->read_value(codeNo, 2);
        uint16_t year  = (telegram->message_data[4] & 0x7F) + 2000;
        uint8_t  month = telegram->message_data[5];
        uint8_t  day   = telegram->message_data[7];
        uint8_t  hour  = telegram->message_data[6];
        uint8_t  min   = telegram->message_data[8];
        uint32_t date  = (year - 2000) * 535680UL + month * 44640UL + day * 1440UL + hour * 60 + min;
        // store only the newest code from telegrams 10 and 11
        if (date > lastCodeDate_) {
            snprintf_P(lastCode_, sizeof(lastCode_), PSTR("%s(%d) %02d.%02d.%d %02d:%02d"), code, codeNo, day, month, year, hour, min);
            lastCodeDate_ = date;
        }
    }
}

// 0x15
void Boiler::process_UBAMaintenanceData(std::shared_ptr<const Telegram> telegram) {
    if (telegram->offset > 0 || telegram->message_length < 5) {
        return;
    }
    // first byte: Maintenance messages (0 = none, 1 = by operating hours, 2 = by date)
    changed_ |= telegram->read_value(maintenanceType_, 0);
    changed_ |= telegram->read_value(maintenanceTime_, 1);
    uint8_t day   = telegram->message_data[2];
    uint8_t month = telegram->message_data[3];
    uint8_t year  = telegram->message_data[4];
    if (day > 0 && month > 0) {
        snprintf_P(maintenanceDate_, sizeof(maintenanceDate_), PSTR("%02d.%02d.%04d"), day, month, year + 2000);
    }
}

// Set the warm water temperature 0x33
bool Boiler::set_warmwater_temp(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler warm water temperature: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler warm water temperature to %d C"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParameterWWPlus, 6, v, EMS_TYPE_UBAParameterWWPlus);
    } else {
        write_command(EMS_TYPE_UBAParameterWW, 2, v, EMS_TYPE_UBAParameterWW); // read seltemp back
        write_command(EMS_TYPE_UBAFlags, 3, v, 0x34);                          // for i9000, see #397, read setTemp
    }

    return true;
}

// flow temp
bool Boiler::set_flow_temp(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler flow temperature: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler flow temperature to %d C"), v);
    write_command(EMS_TYPE_UBASetPoints, 0, v, EMS_TYPE_UBASetPoints);

    return true;
}

// set min boiler output
bool Boiler::set_heating_activated(const char * value, const int8_t id) {
    bool v = false;
    if (!Helpers::value2bool(value, v)) {
        LOG_WARNING(F("Set boiler heating: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler heating %s"), v ? "on" : "off");
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 0, v ? 0x01 : 0, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 0, v ? 0xFF : 0, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set heating maximum temperature
bool Boiler::set_heating_temp(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler heating temperature: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler heating temperature to %d C"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 1, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 1, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set min boiler output
bool Boiler::set_min_power(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler min power: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler min power to %d %%"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 5, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 3, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set max temp
bool Boiler::set_max_power(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler max power: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler max power to %d %%"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 4, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 2, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set min pump modulation
bool Boiler::set_min_pump(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set pump min: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting pump min to %d %%"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 14, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 10, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set max pump modulation
bool Boiler::set_max_pump(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set pump max: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting pump max to %d %%"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 13, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 9, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set boiler on hysteresis
bool Boiler::set_hyst_on(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler hysteresis: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler hysteresis on to %d C"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 9, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 5, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set boiler off hysteresis
bool Boiler::set_hyst_off(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler hysteresis: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler hysteresis off to %d C"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 8, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 4, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set min burner period
bool Boiler::set_burn_period(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set burner min. period: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting burner min. period to %d min"), v);
    if (get_toggle_fetch(EMS_TYPE_UBAParametersPlus)) {
        write_command(EMS_TYPE_UBAParametersPlus, 10, v, EMS_TYPE_UBAParametersPlus);
    } else {
        write_command(EMS_TYPE_UBAParameters, 6, v, EMS_TYPE_UBAParameters);
    }

    return true;
}

// set pump delay
bool Boiler::set_pump_delay(const char * value, const int8_t id) {
    int v = 0;
    if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set boiler pump delay: Invalid value"));
        return false;
    }

    if (get_toggle_fetch(EMS_TYPE_UBAParameters)) {
        LOG_INFO(F("Setting boiler pump delay to %d min"), v);
        write_command(EMS_TYPE_UBAParameters, 8, v, EMS_TYPE_UBAParameters);
        return true;
    }

    return false;
}

// note some boilers do not have this setting, than it's done by thermostat
// on a RC35 it's by EMSESP::send_write_request(0x37, 0x10, 2, &set, 1, 0); (set is 1,2,3) 1=hot, 2=eco, 3=intelligent
bool Boiler::set_warmwater_mode(const char * value, const int8_t id) {
    uint8_t set;
    if (!Helpers::value2enum(value, set, {F("hot"), F("eco"), F("intelligent")})) {
        LOG_WARNING(F("Set boiler warm water mode: Invalid value"));
        return false;
    }

    if (!get_toggle_fetch(EMS_TYPE_UBAParameterWW)) {
        return false;
    }

    if (set == 0) {
        LOG_INFO(F("Setting boiler warm water to Hot"));
    } else if (set == 1) {
        LOG_INFO(F("Setting boiler warm water to Eco"));
        set = 0xD8;
    } else if (set == 2) {
        LOG_INFO(F("Setting boiler warm water to Intelligent"));
        set = 0xEC;
    } else {
        return false; // do nothing
    }

    write_command(EMS_TYPE_UBAParameterWW, 9, set, EMS_TYPE_UBAParameterWW);
    return true;
}

// turn on/off warm water
bool Boiler::set_warmwater_activated(const char * value, const int8_t id) {
    bool v = false;
    if (!Helpers::value2bool(value, v)) {
        LOG_WARNING(F("Set boiler warm water active: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting boiler warm water active %s"), v ? "on" : "off");

    // https://github.com/emsesp/EMS-ESP/issues/268
    uint8_t n;
    if (EMSbus::is_ht3()) {
        n = (v ? 0x08 : 0x00); // 0x08 is on, 0x00 is off
    } else {
        n = (v ? 0xFF : 0x00); // 0xFF is on, 0x00 is off
    }

    if (get_toggle_fetch(EMS_TYPE_UBAParameterWWPlus)) {
        write_command(EMS_TYPE_UBAParameterWWPlus, 1, v ? 1 : 0, EMS_TYPE_UBAParameterWWPlus);
    } else {
        write_command(EMS_TYPE_UBAParameterWW, 1, n, 0x34);
    }

    return true;
}

// Activate / De-activate the Warm Tap Water
// Note: Using the type 0x1D to put the boiler into Test mode. This may be shown on the boiler with a flashing 'T'
bool Boiler::set_tapwarmwater_activated(const char * value, const int8_t id) {
    bool v = false;
    if (!Helpers::value2bool(value, v)) {
        LOG_WARNING(F("Set warm tap water: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting warm tap water %s"), v ? "on" : "off");
    uint8_t message_data[EMS_MAX_TELEGRAM_MESSAGE_LENGTH];
    for (uint8_t i = 0; i < sizeof(message_data); i++) {
        message_data[i] = 0x00;
    }

    // we use the special test mode 0x1D for this. Setting the first data to 5A puts the system into test mode and
    // a setting of 0x00 puts it back into normal operating mode
    // when in test mode we're able to mess around with the 3-way valve settings
    if (!v) {
        // on
        message_data[0] = 0x5A; // test mode on
        message_data[1] = 0x00; // burner output 0%
        message_data[3] = 0x64; // boiler pump capacity 100%
        message_data[4] = 0xFF; // 3-way valve hot water only
    } else {
        // get out of test mode. Send all zeros.
        // telegram: 0B 08 1D 00 00
    }

    write_command(EMS_TYPE_UBAFunctionTest, 0, message_data, sizeof(message_data), 0);

    return true;
}

// Activate / De-activate One Time warm water 0x35
// true = on, false = off
// See also https://github.com/emsesp/EMS-ESP/issues/341#issuecomment-596245458 for Junkers
bool Boiler::set_warmwater_onetime(const char * value, const int8_t id) {
    bool v = false;
    if (!Helpers::value2bool(value, v)) {
        LOG_WARNING(F("Set warm water OneTime loading: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting warm water OneTime loading %s"), v ? "on" : "off");
    if (get_toggle_fetch(EMS_TYPE_UBAParameterWWPlus)) {
        write_command(EMS_TYPE_UBAFlags, 0, (v ? 0x22 : 0x02), 0xE9); // not sure if this is in flags
    } else {
        write_command(EMS_TYPE_UBAFlags, 0, (v ? 0x22 : 0x02), 0x34);
    }

    return true;
}

// Activate / De-activate circulation of warm water 0x35
// true = on, false = off
bool Boiler::set_warmwater_circulation(const char * value, const int8_t id) {
    bool v = false;
    if (!Helpers::value2bool(value, v)) {
        LOG_WARNING(F("Set warm water circulation: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting warm water circulation %s"), v ? "on" : "off");
    if (get_toggle_fetch(EMS_TYPE_UBAParameterWWPlus)) {
        write_command(EMS_TYPE_UBAFlags, 1, (v ? 0x22 : 0x02), 0xE9); // not sure if this is in flags
    } else {
        write_command(EMS_TYPE_UBAFlags, 1, (v ? 0x22 : 0x02), 0x34);
    }

    return true;
}

// configuration of warm water circulation pump
bool Boiler::set_warmwater_circulation_pump(const char * value, const int8_t id) {
    bool v = false;
    if (!Helpers::value2bool(value, v)) {
        LOG_WARNING(F("Set warm water circulation pump: Invalid value"));
        return false;
    }

    LOG_INFO(F("Setting warm water circulation pump %s"), v ? "on" : "off");

    if (get_toggle_fetch(EMS_TYPE_UBAParameterWWPlus)) {
        write_command(EMS_TYPE_UBAParameterWWPlus, 10, v ? 0x01 : 0x00, EMS_TYPE_UBAParameterWWPlus);
    } else {
        write_command(EMS_TYPE_UBAParameterWW, 6, v ? 0xFF : 0x00, EMS_TYPE_UBAParameterWW);
    }

    return true;
}

// Set the mode of circulation, 1x3min, ... 6x3min, continuos
// true = on, false = off
bool Boiler::set_warmwater_circulation_mode(const char * value, const int8_t id) {
    // int v = 0;
    uint8_t v;
    if (!Helpers::value2enum(value, v, {F("off"), F("1"), F("2"), F("3"), F("4"), F("5"), F("6"), F("continous")})) {
        // if (!Helpers::value2number(value, v)) {
        LOG_WARNING(F("Set warm water circulation mode: Invalid value"));
        return false;
    }

    if (v == 0) {
        LOG_INFO(F("Seting warm water circulation mode off"));
    } else if (v < 7) {
        LOG_INFO(F("Setting warm water circulation mode %dx3min"), v);
    } else if (v == 7) {
        LOG_INFO(F("Setting warm water circulation mode continuos"));
    } else {
        LOG_WARNING(F("Set warm water circulation mode: Invalid value"));
        return false;
    }

    if (get_toggle_fetch(EMS_TYPE_UBAParameterWWPlus)) {
        write_command(EMS_TYPE_UBAParameterWWPlus, 11, v, EMS_TYPE_UBAParameterWWPlus);
    } else {
        write_command(EMS_TYPE_UBAParameterWW, 7, v, EMS_TYPE_UBAParameterWW);
    }

    return true;
}

/*
// Reset command
// 0 & 1        Reset-Mode (Manual, others), 5A resets Lxx error?
// 8            reset maintenance message Hxx, send FF
// 12 & 13      Reset that Error-memory

bool Boiler::set_reset(const char * value, const int8_t id) {
    std::string s(12, '\0');
    if (!Helpers::value2string(value, s)) {
        return false;
    }
    if (s == "maintenance") {
        LOG_INFO(F("Reset boiler maintenance message"));
        write_command(0x05, 0x08, 0xFF, 0x1C);
        return true;
    } else if (s == "error") {
        LOG_INFO(F("Reset boiler error message"));
        write_command(0x05, 0x00, 0x5A); error reset
        return true;
    }
    return false;
}
*/

//maintenance
bool Boiler::set_maintenance(const char * value, const int8_t id) {
    std::string s(12, '\0');
    if (Helpers::value2string(value, s)) {
        if (s == "reset") {
            LOG_INFO(F("Reset boiler maintenance message"));
            write_command(0x05, 0x08, 0xFF, 0x1C);
            return true;
        }
    }

    if (strlen(value) == 10) { // date
        uint8_t day   = (value[0] - '0') * 10 + (value[1] - '0');
        uint8_t month = (value[3] - '0') * 10 + (value[4] - '0');
        uint8_t year  = (uint8_t)(Helpers::atoint(&value[6]) - 2000);
        if (day > 0 && day < 32 && month > 0 && month < 13) {
            LOG_INFO(F("Setting maintenance date to %02d.%02d.%04d"), day, month, year + 2000);
            uint8_t data[5] = {2, maintenanceTime_, day, month, year};
            write_command(0x15, 0, data, 5, 0x15);
        } else {
            LOG_WARNING(F("Setting maintenance: wrong format %d.%d.%d"), day, month, year + 2000);
            return false;
        }
        return true;
    }

    int hrs;
    if (Helpers::value2number(value, hrs)) {
        if (hrs > 99 && hrs < 25600) {
            LOG_INFO(F("Setting maintenance time %d hours"), hrs);
            uint8_t data[2] = {1, (uint8_t)(hrs / 100)};
            write_command(0x15, 0, data, 2, 0x15);
            return true;
        }
    }

    uint8_t num;
    if (Helpers::value2enum(value, num, {F("off"), F("time"), F("date")})) {
        LOG_INFO(F("Setting maintenance type to %s"), value);
        write_command(0x15, 0, num, 0x15);
        return true;
    }

    LOG_WARNING(F("Setting maintenance: wrong format"));
    return false;
}

} // namespace emsesp
