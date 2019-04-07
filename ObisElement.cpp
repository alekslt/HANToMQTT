
#include "HanReader.h"

void ObisElement::debugString(char* buf) {
  sprintf(buf, "Object: ObisCode:%d.%d.%d.%d.%d.%d  Type:%x(%s) Value: ",
    this->obisOctets[0], this->obisOctets[1], this->obisOctets[2],
    this->obisOctets[3], this->obisOctets[4], this->obisOctets[5],
    this->data->dataType, cosemTypeToString[this->data->dataType].c_str());

  size_t bufLen = strlen(buf);
  switch (this->data->dataType) {
  case TYPE_FLOAT:
    sprintf(&buf[bufLen], "%f %s", this->data->value.value_float, cosemEnumTypeToString[this->enumType].c_str());
    break;
  case TYPE_UINT32:
    sprintf(&buf[bufLen], "%u %s", this->data->value.value_uint, cosemEnumTypeToString[this->enumType].c_str());
    break;

  case TYPE_STRING:
    sprintf(&buf[bufLen], "%.*s", this->data->stringLength, this->data->value.value_string);
    break;

  case TYPE_DATETIME:
    sprintf(&buf[bufLen], "%s Dev:%d ClockStatus: %u", this->data->value.value_dateTime->dateTime,
      this->data->value.value_dateTime->deviation, this->data->value.value_dateTime->clockStatus);
    break;
  }
}

String ObisElement::EnumString() {
  auto search = cosemEnumTypeToString.find(this->enumType);
  if (search != cosemEnumTypeToString.end()) {
    return search->second;
  }
  else {
    return cosemTypeToString[this->data->dataType];
  }
}

String ObisElement::ValueString() {
  char buf[64];
  switch (this->data->dataType) {
  case TYPE_FLOAT:
    sprintf(buf, "%f", this->data->value.value_float);
    break;

  case TYPE_UINT32:
    sprintf(buf, "%u", this->data->value.value_uint);
    break;

  case TYPE_STRING:
    sprintf(buf, "%.*s", this->data->stringLength, this->data->value.value_string);
    break;

  case TYPE_DATETIME:
    return String(this->data->value.value_dateTime->dateTime);
    break;
  }

  return String(buf);
}

String ObisElement::ObisCodeString() {
  char buf[64];
  sprintf(buf, "%d.%d.%d.%d.%d.%d",
    this->obisOctets[0], this->obisOctets[1], this->obisOctets[2],
    this->obisOctets[3], this->obisOctets[4], this->obisOctets[5]);

  return String(buf);
}
/*
void ObisElement::toArduinoJson(JsonObject& json) {
  char buf[64];
  sprintf(buf, "%d.%d.%d.%d.%d.%d",
    this->obisOctets[0], this->obisOctets[1], this->obisOctets[2],
    this->obisOctets[3], this->obisOctets[4], this->obisOctets[5]);

  json["obis_code"] = String(buf);

  switch (this->data->dataType) {
  case TYPE_FLOAT:
    json["value"] = this->data->value.value_float;
    json["type"] = cosemEnumTypeToString[this->enumType];
    break;
  case TYPE_UINT32:
    json["value"] = this->data->value.value_uint;
    json["type"] = cosemEnumTypeToString[this->enumType];
    break;

  case TYPE_STRING:
    sprintf(buf, "%.*s", this->data->stringLength, this->data->value.value_string);
    json["value"] = String(buf);

    break;

  case TYPE_DATETIME:
    json["value"] = String(this->data->value.value_dateTime->dateTime);
    json["deviation"] = this->data->value.value_dateTime->deviation;
    json["clockStatus"] = this->data->value.value_dateTime->clockStatus;
    break;
  }
}
*/
void ObisElement::Reset() {
  if (this->data->dataType == TYPE_DATETIME &&
    this->data->value.value_dateTime != NULL) {

    delete this->data->value.value_dateTime;
  }
  this->data->value.value_uint = 0;
  this->data->dataType = 0;
  this->data->stringLength = 0;
  delete this->data;
  this->data = 0;
  this->enumType = 0;
  this->obisOctets = 0;
}
