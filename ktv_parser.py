# -*- coding: utf-8 -*-
# https://phab.gotokeep.com/T129998
import sys

PREFIX_MODEL = '#'
PREFIX_ARRAY = '*'

TYPE_CHAR = 'char'
TYPE_BYTE = 'byte'
TYPE_INT2 = 'int2'
TYPE_INT4 = 'int4'

TYPE_ARRAY = 'array'
TYPE_MODEL = 'model'
TYPE_MODEL_ARRAY = 'model_array'

TYPE_MAP = {
    TYPE_CHAR: 0x01,
    TYPE_BYTE: 0x02,
    TYPE_INT2: 0x03,
    TYPE_INT4: 0x04,
    TYPE_ARRAY: 0x10,
    TYPE_MODEL: 0x11,
    TYPE_MODEL_ARRAY: 0x12,
}

def parse(lines):
    parsed = []
    modelName = None
    fields = []
    for lineNo, line in enumerate(lines):
        # remove comments
        if line.find('//') > 0:
            line = line[:line.find('//')]
        if line.strip().startswith('//'):
            continue
        isModel, value = parseLine(lineNo, line.strip())
        if isModel == None:
            continue
        if isModel:
            if modelName != None:
                parsed.append((modelName, fields))
            modelName = value
            fields = []
        else:
            fields.append(value)
    if modelName != None:
        parsed.append((modelName, fields))
    return encode(parsed)


def parseLine(lineNo, line):
    if len(line) == 0:
        return None, None
    # model def
    if line[0] == PREFIX_MODEL:
        modelName = line[1:]
        if len(modelName) == 0:
            raise ValueError(
                'Model define error[%d]: model name empty' % (lineNo))
        if not isValidName(modelName):
            raise ValueError(
                'Model define error[%d]: model name should be alphanumeric' % (lineNo))
        return True, modelName
    # field def
    splits = line.split()
    if len(splits) != 2:
        raise ValueError(
            'Field define error[%d]: field definition should be: alias value' % (lineNo))
    alias = splits[0]
    fieldType = splits[1]
    if not isValidName(alias):
        raise ValueError(
            'Field define error[%d]: field alias should be alphanumeric' % (lineNo))
    isArray = False
    if fieldType[0] == PREFIX_ARRAY:
        isArray = True
        fieldType = fieldType[1:]
    return False, (alias, isArray, fieldType)


def encode(models):
    print models
    basicNames = [TYPE_CHAR, TYPE_BYTE, TYPE_INT2, TYPE_INT4]
    modelNames = []
    for model in models:
        modelName = model[0]
        # check reserved words conflict
        if modelName in basicNames:
            raise ValueError(
                'Field define error: model(name=%s) is reserved' % (modelName))
        # check model duplication
        if modelName in modelNames:
            raise ValueError(
                'Field define error: model(name=%s) duplicated' % (modelName))
        # check fields count overflow
        if len(model[1]) == 0:
            raise ValueError(
                'Field define error: model(name=%s) fields should not be empty' % (modelName))
        if len(model[1]) > 255:
            raise ValueError(
                'Field define error: model(name=%s) field counts overflow(>255)' % (modelName))
        modelNames.append(modelName)
    if len(models) > 255:
        raise ValueError(
            'Model define error: model counts overflow(>255)' % (modelName))
    resultBytes = [len(models)]
    for model in models:
        modelName = model[0]
        fields = model[1]
        modelBytes = []
        # model name length + value
        modelBytes.append(len(modelName))
        modelBytes.extend([ord(ch) for ch in modelName])
        # model fields count
        modelBytes.append(len(fields))
        # model fields
        aliasList = []
        for field in fields:
            fieldAlias = field[0]
            isArray = field[1]
            fieldType = field[2]
            if fieldAlias in aliasList:
                raise ValueError(
                    'Field define error: model(name=%s) field alias duplicated(alias=%s)' % (modelName, fieldAlias))
            aliasList.append(fieldAlias)
            # field name length + value
            modelBytes.append(len(fieldAlias))
            modelBytes.extend([ord(ch) for ch in fieldAlias])
            # field type
            isBasic = fieldType in basicNames
            if not isBasic and not fieldType in modelNames:
                raise ValueError(
                    'Field define error: model(name=%s) field undefined(alias=%s, type=%s)' % (modelName, fieldAlias, fieldType))
            if isBasic and not isArray:
                # basic type
                modelBytes.append(TYPE_MAP[fieldType])
                modelBytes.append(0x00)
            elif isBasic and isArray:
                # basic type array
                modelBytes.append(TYPE_MAP[TYPE_ARRAY])
                modelBytes.append(TYPE_MAP[fieldType])
            elif not isBasic and not isArray:
                # model type
                modelBytes.append(TYPE_MAP[TYPE_MODEL])
                modelBytes.append(modelNames.index(fieldType))
            else:
                # model type array
                modelBytes.append(TYPE_MAP[TYPE_MODEL_ARRAY])
                modelBytes.append(modelNames.index(fieldType))
        resultBytes.extend(modelBytes)
        print "\n==== model %s ====" % (modelName)
        print toPrintableBytes(modelBytes)
    return resultBytes


def isValidName(s):
    return s.isalnum() and s[0].isalpha and len(s) < 255


def toPrintableBytes(array):
    return [hex(value) for value in array]


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage: python %s proto_file_path" % (sys.argv[0])
    else:
        fileName = sys.argv[1]
        src = open(fileName, 'r')
        lines = src.readlines()
        result = parse(lines)
        src.close()
        dst = open('%s.bin' % (fileName), 'wb')
        dst.write(bytearray(result))
        dst.close()
