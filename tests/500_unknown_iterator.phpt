--TEST--
Check for protocol buffers unknown field iterator implementation
--FILE--
<?php
require dirname(__FILE__) . DIRECTORY_SEPARATOR . "messages" . DIRECTORY_SEPARATOR . "repeated_string.proto.php";

class UnknownFields extends ProtocolBuffersMessage
{
    protected static $descriptor;

    protected $value;

    protected $_unknown;

    /**
     * get descriptor for protocol buffers
     *
     * @return array
     */
    public static function getDescriptor()
    {
        if (!isset(self::$descriptor)) {
            $desc = new ProtocolBuffersDescriptorBuilder();
            $php = $desc->getOptions()->getExtension("php");
            $php->setProcessUnknownFields(true);

            self::$descriptor = $desc->build();
        }

        return self::$descriptor;
    }
}

$u = new Tutorial_RepeatedString();
$u->addValues("a");
$u->addValues("a");
$u->addValues("a");
$u->addValues("a");
$expect = ProtocolBuffers::encode($u);

$d = UnknownFields::parseFromString($expect);
foreach($d->getUnknownFieldSet() as $field) {
    if ($field->isLengthDelimited()) {
        foreach ($field->getAsLengthDelimitedList() as $l) {
            echo $l . PHP_EOL;
        }
    }
}

--EXPECT--
a
a
a
a
