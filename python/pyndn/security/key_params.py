# -*- Mode:python; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
#
# Copyright (C) 2015-2017 Regents of the University of California.
# Author: Jeff Thompson <jefft0@remap.ucla.edu>
# From ndn-cxx security by Yingdi Yu <yingdi@cs.ucla.edu>.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# A copy of the GNU Lesser General Public License is in the file COPYING.

"""
This module defines KeyParams which is a base class for key parameters. This
also defines the subclasses which are used to store parameters for key
generation.
"""

from pyndn.security.security_types import KeyType
from pyndn.security.key_id_type import KeyIdType
from pyndn.name import Name

class KeyParams(object):
    """
    Create a key generation parameter. This constructor is protected and used by
    subclasses.

    :param keyType: The type for the created key.
    :type keyType: An int from the KeyType enum.
    :param keyIdTypeOrKeyId: If this is a KeyIdType, it is the method for how
      the key id should be generated, which must not be KeyIdType.USER_SPECIFIED.
      If this is a Name.Component, it is the user-specified key ID, in which
      case this sets the keyIdType to KeyIdType.USER_SPECIFIED. (The keyId must
      not be empty.)
    :type keyIdTypeOrKeyId: int from KeyIdType or Name.Component.
    :throws ValueError: If keyIdTypeOrKeyId is a KeyIdType and it is
      KeyIdType.USER_SPECIFIED, or if keyIdTypeOrKeyId is a Name.Component and
      it is empty.
    """
    def __init__(self, keyType, keyIdTypeOrKeyId):
        self._keyType = keyType

        if isinstance(keyIdTypeOrKeyId, Name.Component):
            keyId = keyIdTypeOrKeyId

            if keyId.getValue().size() == 0:
                raise ValueError("KeyParams: keyId is empty")

            self._keyIdType = KeyIdType.USER_SPECIFIED
            self._keyId = keyId
        else:
            keyIdType = keyIdTypeOrKeyId
            
            if keyIdType == KeyIdType.USER_SPECIFIED:
                raise ValueError("KeyParams: KeyIdType is USER_SPECIFIED")

            self._keyIdType = keyIdType
            self._keyId = Name.Component()

    def getKeyType(self):
        return self._keyType

    def getKeyIdType(self):
        return self._keyIdType

    def getKeyId(self):
        return self._keyId

    def setKeyId(self, keyId):
        self._keyId = keyId

class RsaKeyParams(KeyParams):
    """
    Possible forms of the constructor are:
    RsaKeyParams(keyId, size)
    RsaKeyParams(keyId)
    RsaKeyParams(size, keyIdType)
    RsaKeyParams(size)
    RsaKeyParams()
    """
    def __init__(self, keyIdOrSize = None, arg2 = None):
        if isinstance(keyIdOrSize, Name.Component):
            keyId = keyIdOrSize
            super(RsaKeyParams, self).__init__(RsaKeyParams.getType(), keyId)

            if arg2 == None:
                self._size = RsaKeyParams.getDefaultSize()
            else:
                self._size = arg2
        else:
            size = keyIdOrSize
            if size != None:
                keyIdType = arg2 if arg2 != None else KeyIdType.RANDOM
                super(RsaKeyParams, self).__init__(
                  RsaKeyParams.getType(), keyIdType)
                self._size = size
            else:
                super(RsaKeyParams, self).__init__(
                  RsaKeyParams.getType(), KeyIdType.RANDOM)
                self._size = RsaKeyParams.getDefaultSize()

    def getKeySize(self):
        return self._size

    @staticmethod
    def getDefaultSize():
        return 2048

    @staticmethod
    def getType():
        return KeyType.RSA

class EcdsaKeyParams(KeyParams):
    """
    Possible forms of the constructor are:
    EcdsaKeyParams(keyId, size)
    EcdsaKeyParams(keyId)
    EcdsaKeyParams(size, keyIdType)
    EcdsaKeyParams(size)
    EcdsaKeyParams()
    """
    def __init__(self, keyIdOrSize = None, arg2 = None):
        if isinstance(keyIdOrSize, Name.Component):
            keyId = keyIdOrSize
            super(EcdsaKeyParams, self).__init__(EcdsaKeyParams.getType(), keyId)

            if arg2 == None:
                self._size = EcdsaKeyParams.getDefaultSize()
            else:
                self._size = arg2
        else:
            size = keyIdOrSize
            if size != None:
                keyIdType = arg2 if arg2 != None else KeyIdType.RANDOM
                super(EcdsaKeyParams, self).__init__(
                  EcdsaKeyParams.getType(), keyIdType)
                self._size = size
            else:
                super(EcdsaKeyParams, self).__init__(
                  EcdsaKeyParams.getType(), KeyIdType.RANDOM)
                self._size = EcdsaKeyParams.getDefaultSize()

    def getKeySize(self):
        return self._size

    @staticmethod
    def getDefaultSize():
        return 256

    @staticmethod
    def getType():
        return KeyType.ECDSA

class AesKeyParams(KeyParams):
    """
    Possible forms of the constructor are:
    AesKeyParams(keyId, size)
    AesKeyParams(keyId)
    AesKeyParams(size, keyIdType)
    AesKeyParams(size)
    AesKeyParams()
    """
    def __init__(self, keyIdOrSize = None, arg2 = None):
        if isinstance(keyIdOrSize, Name.Component):
            keyId = keyIdOrSize
            super(AesKeyParams, self).__init__(AesKeyParams.getType(), keyId)

            if arg2 == None:
                self._size = AesKeyParams.getDefaultSize()
            else:
                self._size = arg2
        else:
            size = keyIdOrSize
            if size != None:
                keyIdType = arg2 if arg2 != None else KeyIdType.RANDOM
                super(AesKeyParams, self).__init__(
                  AesKeyParams.getType(), keyIdType)
                self._size = size
            else:
                super(AesKeyParams, self).__init__(
                  AesKeyParams.getType(), KeyIdType.RANDOM)
                self._size = AesKeyParams.getDefaultSize()

    def getKeySize(self):
        return self._size

    @staticmethod
    def getDefaultSize():
        return 64

    @staticmethod
    def getType():
        return KeyType.AES
