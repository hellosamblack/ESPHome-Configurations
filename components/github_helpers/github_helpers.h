#pragma once

#include "esphome.h"
#include <ArduinoJson.h>

/**
 * Extract the latest commit SHA from GitHub API response
 * 
 * @param response_data GitHub API response data as a string
 * @return The latest commit SHA (shortened to 10 chars), or empty string on error
 */
std::string extract_latest_commit(const std::string& response_data) {
  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, response_data);
  
  if (err || !doc.is<JsonArray>() || doc.size() == 0) {
    ESP_LOGW("github", "Failed to parse GitHub API response: %s", err.c_str());
    return "";
  }
  
  std::string latest_commit = doc[0]["sha"].as<std::string>();
  if (latest_commit.length() > 10) {
    latest_commit = latest_commit.substr(0, 10); // Shorten for display
  }
  
  return latest_commit;
}

/**
 * Process GitHub API response
 * 
 * @param response_code HTTP response code
 * @param response_data HTTP response body
 * @param latest_commit_sensor Sensor to publish the latest commit
 * @param stored_commit_sensor Sensor to publish the stored commit
 * @param update_available_sensor Binary sensor for update availability
 * @param update_message_sensor Text sensor for update message
 */
void process_github_response(int response_code, 
                            const std::string& response_data,
                            text_sensor::TextSensor* latest_commit_sensor,
                            text_sensor::TextSensor* stored_commit_sensor,
                            binary_sensor::BinarySensor* update_available_sensor,
                            text_sensor::TextSensor* update_message_sensor) {
  // Ensure we got a valid response
  if (response_code != 200) {
    ESP_LOGW("github", "GitHub API request failed with code %d", response_code);
    return;
  }
  
  // Extract the latest commit SHA
  std::string latest_commit = extract_latest_commit(response_data);
  if (latest_commit.empty()) {
    return;
  }
  
  // Update the latest commit sensor
  latest_commit_sensor->publish_state(latest_commit);
  
  // Check if we need to update the stored commit
  std::string stored_commit = stored_commit_sensor->state;
  if (stored_commit.empty()) {
    // First run, store the commit but don't trigger update
    stored_commit_sensor->publish_state(latest_commit);
    update_available_sensor->publish_state(false);
    update_message_sensor->publish_state("No - Initial Setup");
  } else if (stored_commit != latest_commit) {
    // Different commit detected, trigger update notification
    update_available_sensor->publish_state(true);
    update_message_sensor->publish_state("Yes - New Commit: " + latest_commit);
    ESP_LOGI("github", "Update available: %s -> %s", stored_commit.c_str(), latest_commit.c_str());
  } else {
    // Same commit, no update needed
    update_available_sensor->publish_state(false);
    update_message_sensor->publish_state("No - Up to Date");
  }
}

/**
 * Split a string by delimiter
 * 
 * @param str String to split
 * @param delimiter Character to split on
 * @return Vector of string parts
 */
std::vector<std::string> split_string(const std::string& str, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  
  for (char c : str) {
    if (c == delimiter) {
      parts.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  
  parts.push_back(current);
  return parts;
}

/**
 * Extract value after a prefix in a string
 * 
 * @param str Input string
 * @param prefix Prefix to search for
 * @param end_marker Optional end marker to limit extraction
 * @return Extracted value or empty string if not found
 */
std::string extract_value_after(const std::string& str, const std::string& prefix, 
                               const std::string& end_marker = "") {
  auto start_pos = str.find(prefix);
  if (start_pos == std::string::npos) {
    return "";
  }
  
  start_pos += prefix.length();
  
  if (end_marker.empty()) {
    return str.substr(start_pos);
  }
  
  auto end_pos = str.find(end_marker, start_pos);
  if (end_pos == std::string::npos) {
    return str.substr(start_pos);
  }
  
  return str.substr(start_pos, end_pos - start_pos);
}

/**
 * Process chip features string into a formatted list
 * 
 * @param features_str Raw features string
 * @return Formatted features list
 */
std::string format_chip_features(const std::string& features_str) {
  auto features = split_string(features_str, ',');
  
  // Remove any empty features or those starting with 'O'
  features.erase(
    std::remove_if(features.begin(), features.end(), 
      [](const std::string& f) {
        return f.empty() || (f.length() > 0 && f[0] == 'O');
      }
    ),
    features.end()
  );
  
  // Format the features list
  std::string formatted_features;
  for (size_t i = 0; i < features.size(); i++) {
    formatted_features += features[i];
    if (i < features.size() - 1) {
      formatted_features += ", ";
    }
  }
  
  return formatted_features;
}

/**
 * Process device info from debug component
 * 
 * This function handles the parsing of the device information string
 * from the debug component and updates all related sensors.
 */
void process_device_info(const char* raw_value,
                        text_sensor::TextSensor* esphome_version_sensor,
                        text_sensor::TextSensor* chip_model_sensor,
                        text_sensor::TextSensor* chip_features_sensor,
                        text_sensor::TextSensor* chip_cores_sensor,
                        text_sensor::TextSensor* chip_revision_sensor,
                        text_sensor::TextSensor* framework_sensor,
                        text_sensor::TextSensor* esp_idf_version_sensor,
                        text_sensor::TextSensor* efuse_mac_sensor,
                        text_sensor::TextSensor* reset_reason_sensor,
                        text_sensor::TextSensor* wakeup_reason_sensor) {
  // Split the raw value by the pipe delimiter
  auto parts = split_string(std::string(raw_value), '|');
  
  if (parts.size() < 7) {
    ESP_LOGW("device_info", "Not enough parts in device info: %d", parts.size());
    return;
  }
  
  // Process ESPHome version
  esphome_version_sensor->publish_state(parts[0]);
  
  // Find the chip info part
  int chip_info_index = -1;
  for (size_t i = 1; i < parts.size(); i++) {
    if (parts[i].find("Chip:") != std::string::npos) {
      chip_info_index = i;
      break;
    }
  }
  
  if (chip_info_index != -1) {
    // Process chip model
    std::string chip_model = extract_value_after(parts[chip_info_index], "Chip: ", " Features:");
    if (!chip_model.empty()) {
      chip_model_sensor->publish_state(chip_model);
    }
    
    // Process chip features
    std::string features_str = extract_value_after(parts[chip_info_index], "Features:", " Cores:");
    if (!features_str.empty()) {
      chip_features_sensor->publish_state(format_chip_features(features_str));
    }
    
    // Process chip cores
    std::string cores = extract_value_after(parts[chip_info_index], "Cores:", " Revision:");
    if (cores.empty()) {
      cores = extract_value_after(parts[chip_info_index], "Cores:");
    }
    if (!cores.empty()) {
      chip_cores_sensor->publish_state(cores);
    }
    
    // Process chip revision
    std::string revision = extract_value_after(parts[chip_info_index], "Revision:");
    if (!revision.empty()) {
      chip_revision_sensor->publish_state(revision);
    } else {
      chip_revision_sensor->publish_state("Unknown");
    }
  }
  
  // Process framework
  for (const auto& part : parts) {
    if (part.find("Framework:") != std::string::npos) {
      framework_sensor->publish_state(extract_value_after(part, "Framework:"));
    }
    if (part.find("ESP-IDF:") != std::string::npos) {
      esp_idf_version_sensor->publish_state(extract_value_after(part, "ESP-IDF:"));
    }
  }
  
  // Process the last parts (if they exist)
  if (parts.size() >= 3) {
    efuse_mac_sensor->publish_state(extract_value_after(parts[parts.size()-3], "eFuse MAC:"));
  }
  
  if (parts.size() >= 2) {
    reset_reason_sensor->publish_state(extract_value_after(parts[parts.size()-2], "Reset:"));
  }
  
  if (parts.size() >= 1) {
    wakeup_reason_sensor->publish_state(extract_value_after(parts[parts.size()-1], "Wakeup:"));
  }
}