<?php
/**
 * Tutorial_BigTag
 *
 * @message Tutorial.BigTag
 *
 * -*- magic properties -*-
 *
 * @property int $value
 */
class Tutorial_BigTag
{
  protected static $descriptor;

  protected $value;

  public function getValue()
  {
    return $this->value;
  }

  public function setValue($value)
  {
    $this->value = $value;
  }

  /**
   * get descriptor for protocol buffers
   * 
   * @return array
   */
  public static function getDescriptor()
  {
      if (!isset(self::$descriptor)) {
          $desc = new ProtocolBuffersDescriptorBuilder();
          $desc->addField(3081, new ProtocolBuffersFieldDescriptor(array(
              "type"     => ProtocolBuffers::TYPE_INT32,
              "name"     => "value",
              "packable" => false,
              "repeated" => false,
              "default"  => null,
          )));
          self::$descriptor = $desc->build();
      }

      return self::$descriptor;

  }

}

