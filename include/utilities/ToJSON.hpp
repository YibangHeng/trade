#pragma once

#include <google/protobuf/message.h>
#include <nlohmann/json.hpp>
#include <string>

#define PROTOBUF_SYNYAX_VERSION 3

#if PROTOBUF_SYNYAX_VERSION >= 3
    #include <google/protobuf/util/json_util.h>
#endif

namespace trade::utilities
{

class ToJSON
{
public:
    /// Converts protobuf message to inline JSON string.
    /// @param message The message to convert.
    /// @return The JSON string without indentation and newlines.
    std::string operator()(const google::protobuf::Message& message) const
    {
#if PROTOBUF_SYNYAX_VERSION >= 3
        static google::protobuf::util::JsonPrintOptions json_options;

        json_options.add_whitespace                = false;
        json_options.always_print_enums_as_ints    = false;
        json_options.always_print_primitive_fields = true;
        json_options.preserve_proto_field_names    = true;

        std::string json_string;
        MessageToJsonString(message, &json_string, json_options);

        return json_string;
#else
        return proto2_to_json(message).dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace);
#endif
    }

private:
    [[nodiscard]] static nlohmann::json proto2_to_json(const google::protobuf::Message& message) // NOLINT(*-no-recursion)
    {
        nlohmann::json json;

        std::vector<const google::protobuf::FieldDescriptor*> fields;
        message.GetReflection()->ListFields(message, &fields);

        for (const auto& field : fields) {
            switch (field->cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedInt32(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetInt32(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedInt64(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetInt64(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedUInt32(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetUInt32(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedUInt64(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetUInt64(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedDouble(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetDouble(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedFloat(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetFloat(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedBool(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetBool(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedEnum(message, field, i)->name());
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetEnum(message, field)->name();
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(message.GetReflection()->GetRepeatedString(message, field, i));
                    }
                }
                else
                    json[field->name()] = message.GetReflection()->GetString(message, field);
                break;
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                if (field->is_repeated()) {
                    for (int i = 0; i < message.GetReflection()->FieldSize(message, field); i++) {
                        json[field->name()].push_back(proto2_to_json(message.GetReflection()->GetRepeatedMessage(message, field, i)));
                    }
                }
                else {
                    json[field->name()] = proto2_to_json(message.GetReflection()->GetMessage(message, field));
                }
                break;
            default:
                break;
            }
        }

        return json;
    }

#if PROTOBUF_SYNYAX_VERSION >= 3
private:
    static google::protobuf::util::JsonPrintOptions json_options;
#endif
};

} // namespace trade::utilities
